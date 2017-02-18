#include <gtest/gtest.h>

#include <QDir>
#include <QFile>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/io/DeformationTimeSeriesTextFileReader.h>
#include <core/utility/DataExtent.h>

#include "TestEnvironment.h"


class DeformationTimeSeriesTextFileReader_test : public ::testing::Test
{
public:
    using t_FP = float;
    using vtktFPArray = vtkFloatArray;
    using vtkVector2tFP = vtkVector2f;
    using vtkVector3tFP = vtkVector3f;

    static const char * testFileContents()
    {
        static const char contents[] =
            "10 4 cm/year\n"
            "1992.31 1992.40 1992.88 1993.07\n"
            // UTM/WGS85 east, north; temp. int. coherence; deformation velocity; azimuth, range coord; lat., long. coord; residual topo; temporal data...
            "     380000.0 3100000.0                   0.8                 -0.1       160         140   28.0        -16.0        -1.5     0.00010 0.00020 0.00030 0.00040\n"
            "     380001.1 3100001.1                   0.81                -0.11      160.1       140.1 28.1        -16.1        -1.51    0.00011 0.00021 0.00031 0.00041\n"
            "     380002.2 3100002.2                   0.82                -0.12      160.2       140.2 28.2        -16.2        -1.52    0.00012 0.00022 0.00032 0.00042\n";
        return contents;
    }

    static int numberOfDataPoints()
    {
        return 3;
    }
    static int numberOfTimeStamps()
    {
        return 4;
    }
    static const QString & deformationUnit()
    {
        static const QString unit = "cm/year";
        return unit;
    }

    static const QString & testFileName()
    {
        static const QString fileName = QDir(TestEnvironment::testDirPath()).filePath("deformation_timeSeries_txt_format_test_file.txt");
        return fileName;
    }

    void SetUp() override
    {
        TestEnvironment::createTestDir();
        QFile testFile(testFileName());
        testFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        testFile.write(testFileContents());
    }

    void TearDown() override
    {
        TestEnvironment::clearTestDir();
    }

    static const QStringList & timeStamps()
    {
        static const QStringList tss = {
            "1992.31", "1992.40", "1992.88", "1993.07"
        };
        return tss;
    }

    static const std::vector<vtkVector2tFP> & utmWGS85Coords()
    {
        static const std::vector<vtkVector2tFP> coords = {
            { 380000.0f, 3100000.0f },
            { 380001.1f, 3100001.1f },
            { 380002.2f, 3100002.2f },
        };
        return coords;
    }
    static const std::vector<t_FP> & tempIntCoherences()
    {
        static const std::vector<t_FP> coherences = { 0.8f, 0.81f, 0.82f };
        return coherences;
    }
    static const std::vector<vtkVector2tFP> & azimuthRangeCoords()
    {
        static const std::vector<vtkVector2tFP> coords = {
            { 160.0f, 140.0f },
            { 160.1f, 140.1f },
            { 160.2f, 140.2f },
        };
        return coords;
    }
    static const std::vector<vtkVector2tFP> & latLongCoords()
    {
        static const std::vector<vtkVector2tFP> coords = {
            { 28.0f, -16.0f },
            { 28.1f, -16.1f },
            { 28.2f, -16.2f },
        };
        return coords;
    }
    static const std::vector<t_FP> & temporalInterferometricCoherence()
    {
        static const std::vector<t_FP> coherences = { 0.80f, 0.81f, 0.82f };
        return coherences;
    }
    static const std::vector<t_FP> & deformationVelocity()
    {
        static const std::vector<t_FP> velocities = { -0.1f, -0.11f, -0.12f };
        return velocities;
    }
    static const std::vector<t_FP> & residualTopo()
    {
        static const std::vector<t_FP> residuals = { -1.5f, -1.51f, -1.52f };
        return residuals;
    }
    static const std::vector<std::vector<t_FP>> & temporalDeformation()
    {
        static const std::vector<std::vector<t_FP>> deformations = []() {
            // copy paste from text file...
            std::vector<std::vector<t_FP>> d_rows = {
                { 0.00010f, 0.00020f, 0.00030f, 0.00040f },
                { 0.00011f, 0.00021f, 0.00031f, 0.00041f },
                { 0.00012f, 0.00022f, 0.00032f, 0.00042f },
            };
            // .. and transform to required representation [time stamp][data point]
            std::vector<std::vector<t_FP>> d_cols(d_rows.front().size(), std::vector<t_FP>(d_rows.size()));
            for (size_t p = 0; p < d_rows.size(); ++p)
            {
                for (size_t t = 0; t < d_rows.front().size(); ++t)
                {
                    d_cols[t][p] = d_rows[p][t];
                }
            }
            return d_cols;
        }();
        return deformations;
    }
};


TEST_F(DeformationTimeSeriesTextFileReader_test, readInformation)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.readInformation();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validInformation, reader.state());
    ASSERT_EQ(deformationUnit(), reader.deformationUnitString());
    ASSERT_EQ(numberOfTimeStamps(), reader.numTimeSteps());
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readWith_UTM_WGS84_Coordinates)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.setCoordinatesToUse(DeformationTimeSeriesTextFileReader::Coordinate::UTM_WGS84);
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());
    auto readData = reader.generateDataObject();
    ASSERT_TRUE(readData);
    auto readPoly = vtkPolyData::SafeDownCast(readData->dataSet());
    ASSERT_TRUE(readPoly);
    auto points = vtktFPArray::FastDownCast(readPoly->GetPoints()->GetData());
    ASSERT_TRUE(points);
    ASSERT_EQ(3, points->GetNumberOfComponents());

    ASSERT_EQ(numberOfDataPoints(), points->GetNumberOfTuples());
    vtkVector3tFP point;
    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        points->GetTypedTuple(i, point.GetData());
        ASSERT_FLOAT_EQ(utmWGS85Coords()[static_cast<size_t>(i)].GetX(), point.GetX());
        ASSERT_FLOAT_EQ(utmWGS85Coords()[static_cast<size_t>(i)].GetY(), point.GetY());
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readWith_AzimuthRange_Coordinates)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.setCoordinatesToUse(DeformationTimeSeriesTextFileReader::Coordinate::AzimuthRange);
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());
    auto readData = reader.generateDataObject();
    ASSERT_TRUE(readData);
    auto readPoly = vtkPolyData::SafeDownCast(readData->dataSet());
    ASSERT_TRUE(readPoly);
    auto points = vtktFPArray::FastDownCast(readPoly->GetPoints()->GetData());
    ASSERT_TRUE(points);
    ASSERT_EQ(3, points->GetNumberOfComponents());

    ASSERT_EQ(numberOfDataPoints(), points->GetNumberOfTuples());
    vtkVector3tFP point;
    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        points->GetTypedTuple(i, point.GetData());
        ASSERT_FLOAT_EQ(azimuthRangeCoords()[static_cast<size_t>(i)].GetX(), point.GetX());
        ASSERT_FLOAT_EQ(azimuthRangeCoords()[static_cast<size_t>(i)].GetY(), point.GetY());
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readWith_LongitudeLatitude_Coordinates)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.setCoordinatesToUse(DeformationTimeSeriesTextFileReader::Coordinate::LongitudeLatitude);
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());

    auto readData = reader.generateDataObject();
    ASSERT_TRUE(readData);
    auto coordinateTransformable = dynamic_cast<CoordinateTransformableDataObject *>(readData.get());
    ASSERT_TRUE(coordinateTransformable);
    auto readPoly = vtkPolyData::SafeDownCast(readData->dataSet());
    ASSERT_TRUE(readPoly);
    auto points = vtktFPArray::FastDownCast(readPoly->GetPoints()->GetData());
    ASSERT_TRUE(points);
    ASSERT_EQ(3, points->GetNumberOfComponents());

    ASSERT_EQ(numberOfDataPoints(), points->GetNumberOfTuples());
    vtkVector3tFP point;
    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        points->GetTypedTuple(i, point.GetData());
        // swapped latitude/longitude to facilitate rendering
        ASSERT_FLOAT_EQ(latLongCoords()[static_cast<size_t>(i)].GetY(), static_cast<float>(point.GetX()));
        ASSERT_FLOAT_EQ(latLongCoords()[static_cast<size_t>(i)].GetX(), static_cast<float>(point.GetY()));
    }

    const auto refLatLong = coordinateTransformable->coordinateSystem().referencePointLatLong;
    DataBounds bounds;
    readData->dataSet()->GetBounds(bounds.data());
    ASSERT_DOUBLE_EQ(bounds.center()[0], refLatLong[1]);
    ASSERT_DOUBLE_EQ(bounds.center()[1], refLatLong[0]);
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readWith_UTM_WGS84_OthersAsPointAttributes)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.setCoordinatesToUse(DeformationTimeSeriesTextFileReader::Coordinate::UTM_WGS84);
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());

    auto readData = reader.generateDataObject();
    ASSERT_TRUE(readData && readData->dataSet());

    auto & dataSet = *readData->dataSet();

    auto utm_WGS84_array = dataSet.GetPointData()->GetAbstractArray(reader.arrayName_UTM_WGS84());
    ASSERT_FALSE(utm_WGS84_array);
    auto azimuthRangeArray = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_AzimuthRange()));
    ASSERT_TRUE(azimuthRangeArray);
    auto longLat = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_LongitudeLatitude()));
    ASSERT_TRUE(longLat);

    ASSERT_EQ(numberOfDataPoints(), azimuthRangeArray->GetNumberOfTuples());
    ASSERT_EQ(numberOfDataPoints(), longLat->GetNumberOfTuples());
    ASSERT_EQ(3, azimuthRangeArray->GetNumberOfComponents());
    ASSERT_EQ(3, longLat->GetNumberOfComponents());

    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        ASSERT_FLOAT_EQ(azimuthRangeCoords()[static_cast<size_t>(i)].GetX(), azimuthRangeArray->GetTypedComponent(i, 0));
        ASSERT_FLOAT_EQ(azimuthRangeCoords()[static_cast<size_t>(i)].GetY(), azimuthRangeArray->GetTypedComponent(i, 1));
        // swapped latitude/longitude to facilitate rendering
        ASSERT_FLOAT_EQ(latLongCoords()[static_cast<size_t>(i)].GetY(), longLat->GetTypedComponent(i, 0));
        ASSERT_FLOAT_EQ(latLongCoords()[static_cast<size_t>(i)].GetX(), longLat->GetTypedComponent(i, 1));
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readWith_LongLat_OthersAsPointAttributes)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.setCoordinatesToUse(DeformationTimeSeriesTextFileReader::Coordinate::LongitudeLatitude);
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());

    auto readData = reader.generateDataObject();
    ASSERT_TRUE(readData && readData->dataSet());
    auto & dataSet = *readData->dataSet();

    auto utm_WGS84_array = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_UTM_WGS84()));
    ASSERT_TRUE(utm_WGS84_array);
    auto azimuthRangeArray = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_AzimuthRange()));
    ASSERT_TRUE(azimuthRangeArray);
    auto longLatArray = dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_LongitudeLatitude());
    ASSERT_FALSE(longLatArray);

    ASSERT_EQ(numberOfDataPoints(), utm_WGS84_array->GetNumberOfTuples());
    ASSERT_EQ(numberOfDataPoints(), azimuthRangeArray->GetNumberOfTuples());
    ASSERT_EQ(3, utm_WGS84_array->GetNumberOfComponents());
    ASSERT_EQ(3, azimuthRangeArray->GetNumberOfComponents());

    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        ASSERT_FLOAT_EQ(utmWGS85Coords()[static_cast<size_t>(i)].GetX(), utm_WGS84_array->GetTypedComponent(i, 0));
        ASSERT_FLOAT_EQ(utmWGS85Coords()[static_cast<size_t>(i)].GetY(), utm_WGS84_array->GetTypedComponent(i, 1));
        ASSERT_FLOAT_EQ(azimuthRangeCoords()[static_cast<size_t>(i)].GetX(), azimuthRangeArray->GetTypedComponent(i, 0));
        ASSERT_FLOAT_EQ(azimuthRangeCoords()[static_cast<size_t>(i)].GetY(), azimuthRangeArray->GetTypedComponent(i, 1));
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readTemporalInterferometricCoherence)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());

    auto readData = reader.generateDataObject();
    auto & dataSet = *readData->dataSet();

    auto dataArray = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_TemporalInterferometricCoherence()));
    ASSERT_TRUE(dataArray);
    ASSERT_EQ(numberOfDataPoints(), dataArray->GetNumberOfTuples());
    ASSERT_EQ(1, dataArray->GetNumberOfComponents());

    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        ASSERT_FLOAT_EQ(temporalInterferometricCoherence()[static_cast<size_t>(i)],
            dataArray->GetTypedComponent(i, 0));
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readDeformationVelocity)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());

    auto readData = reader.generateDataObject();
    auto & dataSet = *readData->dataSet();

    auto dataArray = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_DeformationVelocity()));
    ASSERT_TRUE(dataArray);
    ASSERT_EQ(numberOfDataPoints(), dataArray->GetNumberOfTuples());
    ASSERT_EQ(1, dataArray->GetNumberOfComponents());

    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        ASSERT_FLOAT_EQ(deformationVelocity()[static_cast<size_t>(i)],
            dataArray->GetTypedComponent(i, 0));
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readResidual)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.readData();

    ASSERT_EQ(DeformationTimeSeriesTextFileReader::validData, reader.state());

    auto readData = reader.generateDataObject();
    auto & dataSet = *readData->dataSet();

    auto dataArray = vtktFPArray::FastDownCast(dataSet.GetPointData()->GetAbstractArray(
        reader.arrayName_ResidualTopography()));
    ASSERT_TRUE(dataArray);
    ASSERT_EQ(numberOfDataPoints(), dataArray->GetNumberOfTuples());
    ASSERT_EQ(1, dataArray->GetNumberOfComponents());

    for (vtkIdType i = 0; i < numberOfDataPoints(); ++i)
    {
        ASSERT_FLOAT_EQ(residualTopo()[static_cast<size_t>(i)],
            dataArray->GetTypedComponent(i, 0));
    }
}

TEST_F(DeformationTimeSeriesTextFileReader_test, readTemporalData)
{
    DeformationTimeSeriesTextFileReader reader;
    reader.setFileName(testFileName());
    reader.readData();

    auto readData = reader.generateDataObject();
    ASSERT_TRUE(readData && readData->dataSet());

    for (int t = 0; t < timeStamps().size(); ++t)
    {
        auto && timeStamp = timeStamps()[t];
        const auto timeStep = timeStamp.toDouble();

        auto producer = readData->processedOutputPort()->GetProducer();
        ASSERT_TRUE(producer->GetExecutive()->UpdateInformation());
        producer->GetOutputInformation(0)->Set(
            vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
            timeStep);
        ASSERT_TRUE(producer->GetExecutive()->Update());
        auto dataSet = vtkDataSet::SafeDownCast(producer->GetOutputDataObject(0));
        ASSERT_TRUE(dataSet);

        auto deformations = vtktFPArray::FastDownCast(dataSet->GetPointData()->GetAbstractArray(
            reader.arrayName_DeformationTimeSeries()));
        ASSERT_TRUE(deformations);
        ASSERT_EQ(numberOfDataPoints(), deformations->GetNumberOfTuples());
        ASSERT_EQ(1, deformations->GetNumberOfComponents());

        for (vtkIdType p = 0; p < numberOfDataPoints(); ++p)
        {
            ASSERT_FLOAT_EQ(temporalDeformation()[static_cast<size_t>(t)][static_cast<size_t>(p)],
                deformations->GetTypedComponent(p, 0));
        }
    }
}
