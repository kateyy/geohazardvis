#include "DeformationTimeSeriesTextFileReader.h"

#include <algorithm>
#include <array>
#include <cassert>

#include <QDebug>
#include <QFileInfo>

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/io/TextFileReader.h>
#include <core/utility/DataExtent.h>


DeformationTimeSeriesTextFileReader::DeformationTimeSeriesTextFileReader(const QString & fileName)
    : m_state{ State::notRead }
    , m_fileName{ fileName }
    , m_coordinatesToUse{ Coordinate::UTM_WGS84 }
    , m_dataOffset{}
    , m_numColumnsBeforeDeformations{ -1 }
    , m_numTimeSteps{ -1 }
    , m_deformationUnitString{}
    , m_readPolyData{}
{
}

DeformationTimeSeriesTextFileReader::DeformationTimeSeriesTextFileReader(DeformationTimeSeriesTextFileReader && other)
    : DeformationTimeSeriesTextFileReader()
{
    *this = std::move(other);
}

DeformationTimeSeriesTextFileReader & DeformationTimeSeriesTextFileReader::operator=(DeformationTimeSeriesTextFileReader && other)
{
    m_state = other.m_state;
    m_fileName = other.m_fileName;
    m_dataOffset = other.m_dataOffset;
    m_coordinatesToUse = other.m_coordinatesToUse;
    m_numColumnsBeforeDeformations = other.m_numColumnsBeforeDeformations;
    m_numTimeSteps = other.m_numTimeSteps;
    m_deformationUnitString = other.m_deformationUnitString;
    m_readPolyData = std::move(other.m_readPolyData);

    return *this;
}

DeformationTimeSeriesTextFileReader::~DeformationTimeSeriesTextFileReader() = default;

void DeformationTimeSeriesTextFileReader::setFileName(const QString & fileName)
{
    if (fileName == m_fileName)
    {
        return;
    }

    clear();
    m_fileName = fileName;
}

const QString & DeformationTimeSeriesTextFileReader::fileName() const
{
    return m_fileName;
}

auto DeformationTimeSeriesTextFileReader::state() const -> State
{
    return m_state;
}

void DeformationTimeSeriesTextFileReader::setCoordinatesToUse(Coordinate coordinate)
{
    if (m_coordinatesToUse == coordinate)
    {
        return;
    }

    clearData();

    m_coordinatesToUse = coordinate;
}

auto DeformationTimeSeriesTextFileReader::coordinateToUse() const -> Coordinate
{
    return m_coordinatesToUse;
}

auto DeformationTimeSeriesTextFileReader::readData() -> State
{
    if (m_state == validData)
    {
        return m_state;
    }
    if (m_state != validInformation)
    {
        if (validInformation != readInformation())
        {
            return m_state;
        }
    }

    if (m_numTimeSteps == 0)
    {
        return setState(validData);
    }

    TextFileReader::StringVectors timestamps;
    auto reader = TextFileReader(m_fileName);
    reader.seekTo(m_dataOffset);
    auto && tsReadFlags = reader.read(timestamps, 1);
    if (tsReadFlags.testFlag(TextFileReader::invalidFile))
    {
        return setState(State::invalidFileName);
    }
    if (!tsReadFlags.testFlag(TextFileReader::successful)
        || tsReadFlags.testFlag(TextFileReader::eof)
        || tsReadFlags.testFlag(TextFileReader::mismatchingColumnCount)
        || tsReadFlags.testFlag(TextFileReader::invalidOffset)
        || timestamps.size() != static_cast<size_t>(m_numTimeSteps)
        || timestamps[0].size() != 1)
    {
        return setState(State::missingData);
    }

    assert(std::all_of(timestamps.cbegin(), timestamps.cend(),
        [] (const decltype(timestamps)::value_type & column) { return column.size() == 1; }));

    // ==> timestamps okay

    TextFileReader::FloatVectors readDataColumns;
    auto && dataReadFlags = reader.read(readDataColumns);

    if (dataReadFlags.testFlag(TextFileReader::invalidFile))
    {
        return setState(invalidFileName);
    }

    const auto expectedNumColumns = static_cast<decltype(readDataColumns.size())>(m_numColumnsBeforeDeformations + m_numTimeSteps);

    if (!dataReadFlags.testFlag(TextFileReader::successful))
    {
        if (dataReadFlags.testFlag(TextFileReader::invalidOffset)
            || dataReadFlags.testFlag(TextFileReader::mismatchingColumnCount))
        {
            return setState(missingData);
        }
        return setState(invalidFileFormat);
    }

    assert(dataReadFlags == (TextFileReader::successful | TextFileReader::eof));

    if (readDataColumns.size() > expectedNumColumns)
    {
        qWarning() << "File contains more data columns than expected.";
        qWarning() << "File name:" << m_fileName
            << "Expected columns (coordinates, attributes + deformations):"
            << m_numColumnsBeforeDeformations + " + " << m_numTimeSteps
            << ", but found" << readDataColumns.size() << "columns";
        readDataColumns.resize(expectedNumColumns);
    }

    const auto numPoints = static_cast<vtkIdType>(readDataColumns[0].size());
    if (static_cast<size_t>(numPoints) != readDataColumns[0].size())
    {
        qWarning() << "Number of points exceeds number of indexable objects, in" << m_fileName;
        return setState(invalidFileFormat);
    }


    // == Parse data columns ==

    struct DataDef
    {
        int firstFileColumn;
        int numFileColumns;
        int numMemColumns;
        const char * arrayName;
    };

    static const auto NumAttributes = 6u;
    static const std::array<DataDef, NumAttributes> attributeDefs = { {
        { 0, 2, 3, arrayName_UTM_WGS84() },
        { 2, 1, 1, arrayName_TemporalInterferometricCoherence() },
        { 3, 1, 1, arrayName_DeformationVelocity() },
        { 4, 2, 3, arrayName_AzimuthRange() },
        { 6, 2, 3, arrayName_LongitudeLatitude() },
        { 8, 1, 1, arrayName_ResidualTopography() },
    } };
    static const auto NumAttributeColumns = [] (const decltype(attributeDefs) & defs) {
        int sum = 0;
        for (auto && def : defs) { sum += def.numFileColumns; }
        return sum;
    }(attributeDefs);

    if (NumAttributeColumns > m_numColumnsBeforeDeformations)
    {
        qWarning() << "Expected more attribute columns than found, in" << m_fileName;
        return setState(invalidFileFormat);
    }

    auto coordArrayIdx = [] (Coordinate coordinate) -> unsigned
    {
        switch (coordinate)
        {
        case Coordinate::UTM_WGS84: return 0u;
        case Coordinate::AzimuthRange: return 3u;
        case Coordinate::LongitudeLatitude: return 4u;
        default:
            assert(false);
            return std::numeric_limits<unsigned>::max();
        }
    };

    if (coordArrayIdx(m_coordinatesToUse) >= attributeDefs.size())
    {
        qWarning() << "Unexpected coordinate type requested";
        return setState(missingData);
    }


    // Swap latitude longitude data columns, so that in visualizations they can be used as:
    //  rendered x = longitude (component 0)
    //  rendered y = latitude  (component 1)
    const auto latitudeFileColIdx = attributeDefs[coordArrayIdx(Coordinate::LongitudeLatitude)].firstFileColumn;
    const auto longitudeFileColIdx = latitudeFileColIdx + 1;
    std::swap(readDataColumns[latitudeFileColIdx], readDataColumns[longitudeFileColIdx]);

    std::array<vtkSmartPointer<vtkFloatArray>, NumAttributes> dataArrays;

    for (unsigned attrIdx = 0; attrIdx < NumAttributes; ++attrIdx)
    {
        auto & def = attributeDefs[attrIdx];
        assert(def.firstFileColumn >= 0
            && def.numFileColumns > 0 && def.numFileColumns <= def.numMemColumns
            && def.arrayName);
        auto & array = dataArrays[attrIdx];

        array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetNumberOfComponents(def.numMemColumns);
        array->SetNumberOfTuples(numPoints);
        array->SetName(def.arrayName);

        for (int c = 0; c < def.numFileColumns; ++c)
        {
            const auto fileColumn = static_cast<size_t>(def.firstFileColumn + c);
            for (vtkIdType i = 0; i < numPoints; ++i)
            {
                const auto fileIdx = static_cast<size_t>(i);
                array->SetTypedComponent(i, c, readDataColumns[fileColumn][fileIdx]);
            }
        }

        // zero initialize dummy columns
        for (int c = def.numFileColumns; c < def.numMemColumns; ++c)
        {
            for (vtkIdType i = 0; i < numPoints; ++i)
            {
                array->SetTypedComponent(i, c, 0.f);
            }
        }
    }

    std::vector<vtkSmartPointer<vtkDataArray>> timestampArrays;
    const auto deformationUnitUtf8 = m_deformationUnitString.toUtf8();
    for (unsigned timestampIdx = 0; timestampIdx < static_cast<unsigned>(m_numTimeSteps); ++timestampIdx)
    {
        const auto & timestampName = timestamps[timestampIdx][0];
        auto && arrayName = deformationArrayBaseName() + timestampName;

        auto array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetNumberOfValues(numPoints);
        array->SetName(arrayName.toUtf8().data());
        if (!deformationUnitUtf8.isEmpty())
        {
            array->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), deformationUnitUtf8.data());
        }

        for (vtkIdType i = 0; i < numPoints; ++i)
        {
            array->SetTypedComponent(i, 0,
                readDataColumns[m_numColumnsBeforeDeformations + timestampIdx][i]);
        }
        timestampArrays.push_back(array);
    }


    // == Setup DataObject and Coordinate System Information ==

    auto verts = vtkSmartPointer<vtkCellArray>::New();
    auto pointIds = std::vector<vtkIdType>(static_cast<size_t>(numPoints));
    for (size_t i = 0; i < static_cast<size_t>(numPoints); ++i)
    {
        pointIds[i] = static_cast<vtkIdType>(i);
    }
    verts->InsertNextCell(numPoints, pointIds.data());

    m_readPolyData = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    m_readPolyData->SetPoints(points);
    m_readPolyData->SetVerts(verts);

    ReferencedCoordinateSystemSpecification dataObjectCoordinateSystem;
    dataObjectCoordinateSystem.geographicSystem = "WGS 84";
    dataObjectCoordinateSystem.globalMetricSystem = "UTM";
    dataObjectCoordinateSystem.referencePointLocalRelative = { 0.5, 0.5 };

    auto coordsArrayLongLat = dataArrays[coordArrayIdx(Coordinate::LongitudeLatitude)];
    auto coordsArrayUTMWGS84 = dataArrays[coordArrayIdx(Coordinate::UTM_WGS84)];

    // determine geographic reference point
    points->SetData(coordsArrayLongLat);
    const auto longLatCenter = DataBounds(m_readPolyData->GetBounds()).convertTo<2>();
    dataObjectCoordinateSystem.referencePointLatLong = { longLatCenter[1], longLatCenter[0] };

    // set requested coordinates as points
    auto currentCoordsArray = dataArrays[coordArrayIdx(m_coordinatesToUse)];
    points->SetData(currentCoordsArray);

    {   // set coordinate array information
        static const auto degreeSign = QString(QChar(0x00B0)).toUtf8();

        auto arraySpec = dataObjectCoordinateSystem;
        arraySpec.type = CoordinateSystemType::geographic;
        arraySpec.writeToInformation(*coordsArrayLongLat->GetInformation());
        coordsArrayLongLat->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), degreeSign.data());

        arraySpec.type = CoordinateSystemType::metricGlobal;
        auto utmArray = dataArrays[coordArrayIdx(Coordinate::UTM_WGS84)];
        arraySpec.writeToInformation(*utmArray->GetInformation());
        utmArray->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), "m");

        arraySpec.type = CoordinateSystemType::unspecified; // current not implemented
        auto azimuthRangeArray = dataArrays[coordArrayIdx(Coordinate::AzimuthRange)];
        arraySpec.writeToInformation(*azimuthRangeArray->GetInformation());
        azimuthRangeArray->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), degreeSign.data());
    }

    switch (m_coordinatesToUse)
    {
    case DeformationTimeSeriesTextFileReader::Coordinate::UTM_WGS84:
        dataObjectCoordinateSystem.type = CoordinateSystemType::metricGlobal;
        break;
    case DeformationTimeSeriesTextFileReader::Coordinate::AzimuthRange:
        break;    // not implemented
    case DeformationTimeSeriesTextFileReader::Coordinate::LongitudeLatitude:
        dataObjectCoordinateSystem.type = CoordinateSystemType::geographic;
        break;
    }

    dataObjectCoordinateSystem.writeToFieldData(*m_readPolyData->GetFieldData());

    auto & pointData = *m_readPolyData->GetPointData();
    for (const auto & array : dataArrays)
    {
        if (array.Get() != currentCoordsArray)
        {
            pointData.AddArray(array);
        }
    }
    for (const auto & array : timestampArrays)
    {
        pointData.AddArray(array);
    }

    return setState(validData);
}

auto DeformationTimeSeriesTextFileReader::readInformation() -> State
{
    TextFileReader::StringVectors strings;
    auto reader = TextFileReader(m_fileName);
    auto && flags = reader.read(strings, 1);
    m_dataOffset = reader.filePos();

    assert(!flags.testFlag(TextFileReader::mismatchingColumnCount)
        && !flags.testFlag(TextFileReader::invalidValue));

    if (flags.testFlag(TextFileReader::invalidFile))
    {
        return setState(State::invalidFileName);
    }
    if (!flags.testFlag(TextFileReader::successful)
        || flags.testFlag(TextFileReader::eof)
        || flags.testFlag(TextFileReader::mismatchingColumnCount)
        || flags.testFlag(TextFileReader::invalidOffset)
        || strings.size() != 3
        || strings[0].empty())
    {
        return setState(State::invalidFileFormat);
    }

    assert(std::all_of(strings.cbegin(), strings.cend(),
        [] (const decltype(strings)::value_type & column) { return column.size() == 1; }));

    bool valueOkay = false;
    const auto oneBasedTemporalDataIndex = strings[0][0].toInt(&valueOkay);
    m_numColumnsBeforeDeformations = oneBasedTemporalDataIndex - 1;
    if (!valueOkay || m_numColumnsBeforeDeformations < 0)
    {
        return setState(State::invalidFileFormat);
    }

    m_numTimeSteps = strings[1][0].toInt(&valueOkay);
    if (!valueOkay || m_numTimeSteps < 0)
    {
        return setState(State::invalidFileFormat);
    }

    m_deformationUnitString = strings[2][0];

    return setState(State::validInformation);
}

std::unique_ptr<DataObject> DeformationTimeSeriesTextFileReader::generateDataObject()
{
    readData();

    if (m_state != State::validData)
    {
        return{};
    }

    auto && name = QFileInfo(m_fileName).baseName();

    return std::make_unique<PointCloudDataObject>(name, *m_readPolyData);
}

void DeformationTimeSeriesTextFileReader::clear()
{
    *this = DeformationTimeSeriesTextFileReader();
}

int DeformationTimeSeriesTextFileReader::numTimeSteps() const
{
    return m_numTimeSteps;
}

const QString & DeformationTimeSeriesTextFileReader::deformationUnitString()
{
    return m_deformationUnitString;
}

const char * DeformationTimeSeriesTextFileReader::arrayName_Coords(Coordinate coordType)
{
    switch (coordType)
    {
    case DeformationTimeSeriesTextFileReader::Coordinate::UTM_WGS84:
        return arrayName_UTM_WGS84();
    case DeformationTimeSeriesTextFileReader::Coordinate::AzimuthRange:
        return arrayName_AzimuthRange();
    case DeformationTimeSeriesTextFileReader::Coordinate::LongitudeLatitude:
        return arrayName_LongitudeLatitude();
    default:
        return nullptr;
    }
}

const char * DeformationTimeSeriesTextFileReader::arrayName_UTM_WGS84()
{
    static const char * const arrayName = "UTM_WGS84";
    return arrayName;
}

const char * DeformationTimeSeriesTextFileReader::arrayName_AzimuthRange()
{
    static const char * const arrayName = "AzimuthRange";
    return arrayName;
}

const char * DeformationTimeSeriesTextFileReader::arrayName_LongitudeLatitude()
{
    static const char * const arrayName = "LatitudeLongitude";
    return arrayName;
}

const char * DeformationTimeSeriesTextFileReader::arrayName_TemporalInterferometricCoherence()
{
    static const char * const arrayName = "Temporal Interferometric Coherence";
    return arrayName;
}

const char * DeformationTimeSeriesTextFileReader::arrayName_DeformationVelocity()
{
    static const char * const arrayName = "Deformation Velocity";
    return arrayName;
}

const char * DeformationTimeSeriesTextFileReader::arrayName_ResidualTopography()
{
    static const char * arrayName = "Residual Topography";
    return arrayName;
}

const QString & DeformationTimeSeriesTextFileReader::deformationArrayBaseName()
{
    static const QString arrayBaseName = "deformation_";
    return arrayBaseName;
}

auto DeformationTimeSeriesTextFileReader::setState(State state) -> State
{
    m_state = state;
    return m_state;
}

void DeformationTimeSeriesTextFileReader::clearData()
{
    m_readPolyData = {};
}
