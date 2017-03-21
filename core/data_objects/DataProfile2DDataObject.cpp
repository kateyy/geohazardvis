#include "DataProfile2DDataObject.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <QDebug>

#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkCellCenters.h>
#include <vtkCellData.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkPassArrays.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkRearrangeFields.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWarpScalar.h>

#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/context2D_data/DataProfile2DContextPlot.h>
#include <core/filters/ImageBlankNonFiniteValuesFilter.h>
#include <core/filters/LineOnCellsSelector2D.h>
#include <core/filters/LineOnPointsSelector2D.h>
#include <core/filters/SetCoordinateSystemInformationFilter.h>
#include <core/filters/SetMaskedPointScalarsToNaNFilter.h>
#include <core/filters/SimplePolyGeoCoordinateTransformFilter.h>
#include <core/table_model/QVtkTableModelProfileData.h>
#include <core/utility/DataExtent.h>
#include <core/utility/macros.h>
#include <core/utility/types_utils.h>
#include <core/utility/vtkvectorhelper.h>


DataProfile2DDataObject::PreprocessingPipeline::PreprocessingPipeline(
        vtkAlgorithm * head, vtkAlgorithm * tail)
    : head{ head }
    , tail{ tail }
{
}

DataProfile2DDataObject::PreprocessingPipeline::~PreprocessingPipeline() = default;

DataProfile2DDataObject::DataProfile2DDataObject(
    const QString & name,
    DataObject & sourceData,
    const QString & scalarsName,
    IndexType scalarsLocation,
    vtkIdType vectorComponent,
    const PreprocessingPipeline & preprocessingPipeline)
    : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    , m_isValid{ false }
    , m_abscissa{ "Position" }
    , m_scalarsName{ scalarsName }
    , m_scalarsLocation{ scalarsLocation }
    , m_vectorComponent{ vectorComponent }
    , m_profileLinePoint1{ 0.0, 0.0 }
    , m_profileLinePoint2{ 1.0, 0.0 }
    , m_doTransformPoints{ false }
    , m_outputTransformation{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
    , m_graphLine{ vtkSmartPointer<vtkWarpScalar>::New() }
{
    // (1) Check if the input DataObject is transformable to a metric coordinate system

    if (!dynamic_cast<CoordinateTransformableDataObject *>(&sourceData))
    {
        qWarning() << "DataObject type not supported for plotting";
        return;
    }
    auto & transformedSource = static_cast<CoordinateTransformableDataObject &>(sourceData);
    const auto & sourceCoordSystem = transformedSource.coordinateSystem();

    if (sourceCoordSystem.isValid())
    {
        auto checkCoords = sourceCoordSystem;
        checkCoords.type = CoordinateSystemType::metricLocal;
        checkCoords.unitOfMeasurement = "km";

        if (transformedSource.canTransformTo(checkCoords))
        {
            m_targetCoordsSpec = checkCoords;
        }
        else
        {
            qWarning() << "DataProfile2DDataObject: source data does not support transformation "
                "to local coordinates. If it is represented in geographic coordinates (degrees), "
                "the resulting plot won't make much sense.";
        }
    }

    m_sourceAlgorithm = m_targetCoordsSpec.isValid()
        ? transformedSource.coordinateTransformedOutputPort(m_targetCoordsSpec)->GetProducer()
        : sourceData.processedOutputPort()->GetProducer();


    // (2) Inject preprocessing pipeline

    if (preprocessingPipeline.head && preprocessingPipeline.tail)
    {
        preprocessingPipeline.head->SetInputConnection(m_sourceAlgorithm->GetOutputPort());
        m_sourceAlgorithm = preprocessingPipeline.tail;
    }
    else
    {
        assert(!preprocessingPipeline.head && !preprocessingPipeline.tail);
    }


    // (3) Check input data type and setup profile pipeline

    auto processedInputData = sourceData.processedOutputDataSet();
    if (!processedInputData || processedInputData->GetNumberOfPoints() == 0)
    {
        qWarning() << "Invalid input data for plot" << name;
        return;
    }

    auto inputImage = vtkImageData::SafeDownCast(processedInputData);
    m_inputIsImage = (inputImage != nullptr);
    if (m_inputIsImage)
    {
        std::array<int, 3> dimensions;
        inputImage->GetDimensions(dimensions.data());
        // only 2D XY-planes are supported
        if (dimensions[2] != 1)
        {
            return;
        }
    }

    auto inputPolyData = vtkPolyData::SafeDownCast(processedInputData);

    if (!m_inputIsImage && !inputPolyData)
    {
        return;
    }


    if (!m_sourceAlgorithm->GetExecutive()->Update())
    {
        qWarning() << "Error while updating plot source data for" << name;
        return;
    }

    // coordinateTransformed* should not change the data set type.
    auto inputData = vtkDataSet::SafeDownCast(m_sourceAlgorithm->GetOutputDataObject(0));
    if (!inputData || inputData->IsTypeOf(processedInputData->GetClassName()))
    {
        assert(false);
        return;
    }

    const auto c_scalarsName = scalarsName.toUtf8();
    const auto location = IndexType_util(scalarsLocation);
    auto scalars = location.extractArray(inputData, c_scalarsName.data());
    if (scalars == nullptr || scalars->GetNumberOfComponents() < vectorComponent)
    {
        return;
    }

    // remove all other fields
    auto filterFields = vtkSmartPointer<vtkPassArrays>::New();
    filterFields->AddArray(location.toVtkAssignAttribute_AttributeLocation(), c_scalarsName.data());
    filterFields->UseFieldTypesOn();
    filterFields->AddFieldType(vtkDataObject::POINT);
    filterFields->AddFieldType(vtkDataObject::CELL);
    filterFields->AddFieldType(vtkDataObject::FIELD);
    filterFields->SetInputConnection(m_sourceAlgorithm->GetOutputPort());

    // Do not discard or distort normal and vector arrays.
    // vtkRearrangeFields will remove the previous attribute assignment, if such exists
    auto unassignField = vtkSmartPointer<vtkRearrangeFields>::New();
    const auto rearrangeLoc = location.toVtkRearrangeFields_FieldLocation();
    unassignField->AddOperation(vtkRearrangeFields::MOVE, c_scalarsName.data(), rearrangeLoc, rearrangeLoc);
    unassignField->SetInputConnection(filterFields->GetOutputPort());

    auto assignScalars = vtkSmartPointer<vtkAssignAttribute>::New();
    assignScalars->Assign(c_scalarsName.data(), vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::POINT_DATA);
    assignScalars->SetInputConnection(unassignField->GetOutputPort());

    if (m_inputIsImage)
    {
        m_probeLine = vtkSmartPointer<vtkLineSource>::New();

        // In case the image contains NaN's, blank them to be correctly processed by vtkProbeFilter
        auto blankNaNs = vtkSmartPointer<ImageBlankNonFiniteValuesFilter>::New();
        blankNaNs->SetInputConnection(assignScalars->GetOutputPort());

        auto imageProbe = vtkSmartPointer<vtkProbeFilter>::New();
        imageProbe->SetInputConnection(m_probeLine->GetOutputPort());
        imageProbe->SetSourceConnection(blankNaNs->GetOutputPort());

        // restore NaN values after probing (based on invalid point mask)
        auto invalidToNaN = vtkSmartPointer<SetMaskedPointScalarsToNaNFilter>::New();
        invalidToNaN->SetInputConnection(imageProbe->GetOutputPort());

        m_outputTransformation->SetInputConnection(invalidToNaN->GetOutputPort());
    }
    else if (inputPolyData && inputPolyData->GetPoints() && inputPolyData->GetPoints()->GetNumberOfPoints() > 0)
    {
        // Polygonal/triangular data
        if (inputPolyData->GetPolys() && inputPolyData->GetPolys()->GetNumberOfCells() > 0)
        {
            // Cannot reuse PolyDataObject::cellCenters if transformed coordinates are used.
            // So keep it simple and compute the cell centers here on the fly.
            auto polygonCenters = vtkSmartPointer<vtkCellCenters>::New();
            polygonCenters->SetInputConnection(assignScalars->GetOutputPort());

            m_polyCentroidsSelector = vtkSmartPointer<LineOnCellsSelector2D>::New();
            m_polyCentroidsSelector->SetSorting(LineOnCellsSelector2D::SortPoints);
            m_polyCentroidsSelector->PassDistanceToLineOff();
            m_polyCentroidsSelector->PassPositionOnLineOff();
            m_polyCentroidsSelector->SetInputConnection(assignScalars->GetOutputPort());
            m_polyCentroidsSelector->SetCellCentersConnection(polygonCenters->GetOutputPort());

            m_outputTransformation->SetInputConnection(m_polyCentroidsSelector->GetOutputPort(1));
        }
        // Point cloud data
        else if (inputPolyData->GetVerts() && inputPolyData->GetVerts()->GetNumberOfCells() > 0)
        {
            m_polyPointsSelector = vtkSmartPointer<LineOnPointsSelector2D>::New();
            m_polyPointsSelector->SetSorting(LineOnPointsSelector2D::SortPoints);
            m_polyPointsSelector->PassDistanceToLineOff();
            m_polyPointsSelector->PassPositionOnLineOff();
            m_polyPointsSelector->SetInputConnection(unassignField->GetOutputPort());

            m_outputTransformation->SetInputConnection(m_polyPointsSelector->GetOutputPort(1));
        }
        else
        {
            qWarning() << "Plotting not supported for the data set type:" << sourceData.name();
            return;
        }
    }
    else
    {
        qWarning() << "Plotting not supported for the data set type:" << sourceData.name();
        return;
    }

    auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
    // output geometry is always points
    assign->Assign(c_scalarsName.data(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    assign->SetInputConnection(m_outputTransformation->GetOutputPort());

    m_graphLine->UseNormalOn();
    m_graphLine->SetNormal(0, 1, 0);
    m_graphLine->SetInputConnection(assign->GetOutputPort());

    m_isValid = true;
}

DataProfile2DDataObject::~DataProfile2DDataObject() = default;

std::unique_ptr<DataObject> DataProfile2DDataObject::newInstance(const QString & /*name*/, vtkDataSet * /*dataSet*/) const
{
    return{};
}

bool DataProfile2DDataObject::isValid() const
{
    return m_isValid;
}

bool DataProfile2DDataObject::is3D() const
{
    return false;
}

IndexType DataProfile2DDataObject::defaultAttributeLocation() const
{
    return IndexType::points;
}

std::unique_ptr<Context2DData> DataProfile2DDataObject::createContextData()
{
    return std::make_unique<DataProfile2DContextPlot>(*this);
}

const QString & DataProfile2DDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & DataProfile2DDataObject::dataTypeName_s()
{
    static const QString name{ "Data Set Profile (2D)" };
    return name;
}

const QString & DataProfile2DDataObject::abscissa() const
{
    return m_abscissa;
}

const QString & DataProfile2DDataObject::scalarsName() const
{
    return m_scalarsName;
}

IndexType DataProfile2DDataObject::scalarsLocation() const
{
    return m_scalarsLocation;
}

vtkIdType DataProfile2DDataObject::vectorComponent() const
{
    return m_vectorComponent;
}

ValueRange<> DataProfile2DDataObject::scalarRange()
{
    // x-y-plot -> value range on the y axis
    return ValueRange<>(&processedOutputDataSet()->GetBounds()[2]);
}

int DataProfile2DDataObject::numberOfScalars()
{
    return static_cast<int>(processedOutputDataSet()->GetNumberOfPoints());
}

const vtkVector2d & DataProfile2DDataObject::profileLinePoint1() const
{
    return m_profileLinePoint1;
}

const vtkVector2d & DataProfile2DDataObject::profileLinePoint2() const
{
    return m_profileLinePoint2;
}

void DataProfile2DDataObject::setProfileLinePoints(const vtkVector2d & point1, const vtkVector2d & point2)
{
    if (m_profileLinePoint1 == point1 && m_profileLinePoint2 == point2)
    {
        return;
    }

    m_profileLinePoint1 = point1;
    m_profileLinePoint2 = point2;

    updateLinePoints();
}

void DataProfile2DDataObject::setPointsCoordinateSystem(const CoordinateSystemSpecification & coordsSpec)
{
    if (m_profileLinePointsCoordsSpec == coordsSpec)
    {
        return;
    }

    m_profileLinePointsCoordsSpec = coordsSpec;

    updateLinePointsTransform();
    updateLinePoints();
}

const CoordinateSystemSpecification & DataProfile2DDataObject::pointsCoordinateSystem() const
{
    return m_profileLinePointsCoordsSpec;
}

vtkAlgorithmOutput * DataProfile2DDataObject::processedOutputPortInternal()
{
    return m_graphLine->GetOutputPort();
}

std::unique_ptr<QVtkTableModel> DataProfile2DDataObject::createTableModel()
{
    auto tableModel = std::make_unique<QVtkTableModelProfileData>();
    tableModel->setDataObject(this);

    return std::move(tableModel);
}

void DataProfile2DDataObject::updateLinePointsTransform()
{
    struct TransformRequiredCheck
    {
        explicit TransformRequiredCheck(bool & value) : ptr{ &value } { }
        ~TransformRequiredCheck() { if (ptr) *ptr = false; }
        void validate() { *ptr = true; ptr = nullptr; }
    private:
        bool * ptr;
    } transformPointsCheck(m_doTransformPoints);

    if (!m_targetCoordsSpec.isValid())
    {
        // Source data coordinate system is not defined, so assume all coordinates to be untransformed.
        return;
    }

    if (!m_profileLinePointsCoordsSpec.isValid() || m_profileLinePointsCoordsSpec.isUnspecified())
    {
        qWarning() << "Invalid point coordinate system passed to DataProfile2DDataObject. "
            "Line points won't be transformed at all.";
        return;
    }

    if (m_profileLinePointsCoordsSpec == m_targetCoordsSpec) // points already in target system
    {
        return;
    }

    if (m_profileLinePointsCoordsSpec.type != m_targetCoordsSpec.type)
    {
        if (!SimplePolyGeoCoordinateTransformFilter::isConversionSupported(
            m_profileLinePointsCoordsSpec.type,
            m_targetCoordsSpec.type))
        {
            qWarning() << "Unsupported point coordinate system passed to DataProfile2DDataObject. "
                "Line points won't be transformed at all.";
            return;
        }
    }

    // A transformation of the line points is required and supported, so create and configure the filter.
    if (!m_pointsTransformFilter)
    {
        m_pointsSetCoordsSpecFilter = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(2);
        poly->SetPoints(points);
        m_pointsSetCoordsSpecFilter->SetInputData(poly);

        m_pointsTransformFilter = vtkSmartPointer<SimplePolyGeoCoordinateTransformFilter>::New();
        m_pointsTransformFilter->SetInputConnection(m_pointsSetCoordsSpecFilter->GetOutputPort());
    }

    m_pointsTransformFilter->SetTargetCoordinateSystemType(m_targetCoordsSpec.type);

    transformPointsCheck.validate();
}

void DataProfile2DDataObject::updateLinePoints()
{
    vtkVector2d p1, p2;

    if (m_doTransformPoints && m_pointsTransformFilter)
    {
        assert(m_pointsSetCoordsSpecFilter);
        auto inPoly = vtkPolyData::SafeDownCast(m_pointsSetCoordsSpecFilter->GetInput());
        assert(inPoly);
        auto points = inPoly->GetPoints();
        assert(points && points->GetNumberOfPoints() == 2);
        points->SetPoint(0, convertTo<3>(m_profileLinePoint1).GetData());
        points->SetPoint(1, convertTo<3>(m_profileLinePoint2).GetData());
        points->Modified();
        const auto referencedSpec = ReferencedCoordinateSystemSpecification(m_profileLinePointsCoordsSpec,
            m_targetCoordsSpec.referencePointLatLong,
            m_targetCoordsSpec.referencePointLocalRelative);
        m_pointsSetCoordsSpecFilter->SetCoordinateSystemSpec(referencedSpec);

        DEBUG_ONLY(int result = )
            m_pointsTransformFilter->GetExecutive()->Update();
        assert(result);
        auto outPoly = vtkPolyData::SafeDownCast(m_pointsTransformFilter->GetOutputDataObject(0));
        assert(outPoly);
        auto outPoints = outPoly->GetPoints();
        assert(outPoints && outPoints->GetNumberOfPoints() == 2);

        p1 = vtkVector2d(outPoints->GetPoint(0));
        p2 = vtkVector2d(outPoints->GetPoint(1));
    }
    else
    {
        p1 = m_profileLinePoint1;
        p2 = m_profileLinePoint2;
    }

    if (!m_sourceAlgorithm->GetExecutive()->Update())
    {
        qWarning() << "Error while updating plot source data for" << name();
        return;
    }

    auto transformedSourceData = vtkSmartPointer<vtkDataSet>(vtkDataSet::SafeDownCast(
        m_sourceAlgorithm->GetOutputDataObject(0)));

    const auto probeVector = p2 - p1;

    if (auto image = vtkImageData::SafeDownCast(transformedSourceData))
    {
        m_probeLine->SetPoint1(convertTo<3>(p1).GetData());
        m_probeLine->SetPoint2(convertTo<3>(p2).GetData());


        vtkVector3d pointSpacing;
        image->GetSpacing(pointSpacing.GetData());
        const auto xyLength = probeVector / convertTo<2>(pointSpacing);
        const int numProbePoints = static_cast<int>(xyLength.Norm());

        m_probeLine->SetResolution(numProbePoints);
    }
    else if (m_polyCentroidsSelector)
    {
        m_polyCentroidsSelector->SetStartPoint(p1);
        m_polyCentroidsSelector->SetEndPoint(p2);
    }
    else if (m_polyPointsSelector)
    {
        m_polyPointsSelector->SetStartPoint(p1);
        m_polyPointsSelector->SetEndPoint(p2);
    }
    else
    {
        assert(false);
    }


    auto m = vtkSmartPointer<vtkTransform>::New();
    m->PostMultiply();

    // move to origin
    m->Translate((-convertTo<3>(p1)).GetData());
    // align to x axis
    m->RotateZ(
        -vtkMath::DegreesFromRadians(std::atan2(probeVector[1], probeVector[0])));

    m_outputTransformation->SetTransform(m);

    emit dataChanged();
    emit boundsChanged();
}
