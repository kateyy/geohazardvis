#include "GeographicTransformationFilter.h"

#include <cassert>
#include <cmath>

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArrayAccessor.h>
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkSmartPointer.h>
#include <vtkSMPTools.h>
#include <vtkVector.h>

#include <core/ThirdParty/proj4_include.h>

#include <core/utility/DataExtent.h>
#include <core/utility/macros.h>
#include <core/utility/mathhelper.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{

template<unsigned ModifyComponents>
struct ArrayScaleShiftWorker
{
    vtkVector<double, ModifyComponents> Scale;
    vtkVector<double, ModifyComponents> Shift;

    template<typename CoordsArrayType>
    void operator()(CoordsArrayType * coordinates)
    {
        static_assert(3 >= ModifyComponents, "");
        VTK_ASSUME(coordinates->GetNumberOfComponents() == 3);
        vtkDataArrayAccessor<CoordsArrayType> coords(coordinates);
        using ValueType = typename vtkDataArrayAccessor<CoordsArrayType>::APIType;

        const auto scale = convertTo<ValueType>(this->Scale);
        const auto shift = convertTo<ValueType>(this->Shift);

        vtkSMPTools::For(0, coordinates->GetNumberOfTuples(), [coords, scale, shift]
        (const vtkIdType begin, const vtkIdType end)
        {
            vtkVector3<ValueType> dataVec;
            auto & vec = reinterpret_cast<vtkVector<ValueType, ModifyComponents> &>(dataVec);
            for (vtkIdType i = begin; i < end; ++i)
            {
                coords.Get(i, dataVec.GetData());
                vec = vec * scale + shift;
                coords.Set(i, dataVec.GetData());
            }
        });
    }
};

template<unsigned ModifyComponents>
void scaleShiftArray(vtkDataArray & array,
    const vtkVector<double, ModifyComponents> & scale, const vtkVector<double, ModifyComponents> & shift)
{
    ArrayScaleShiftWorker<ModifyComponents> worker;
    worker.Scale = scale;
    worker.Shift = shift;
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    if (!Dispatcher::Execute(&array, worker))
    {
        worker(&array);
    }
}

template<unsigned int ModifyComponents>
void scaleShiftDataSet(vtkDataSet & dataSet,
    const vtkVector<double, ModifyComponents> & scale,
    const vtkVector<double, ModifyComponents> & shift)
{
    static_assert(ModifyComponents <= 3, "");
    if (auto pointSet = vtkPointSet::SafeDownCast(&dataSet))
    {
        auto pointCoords = pointSet->GetPoints()->GetData();
        assert(pointCoords);
        scaleShiftArray(*pointCoords, scale, shift);
        return;
    }

    if (auto image = vtkImageData::SafeDownCast(&dataSet))
    {
        std::array<double, 3> spacing, origin;
        image->GetSpacing(spacing.data());
        image->GetOrigin(origin.data());
        for (unsigned int i = 0; i < ModifyComponents; ++i)
        {
            spacing[i] *= scale[i];
            origin[i] = origin[i] * scale[i] + shift[i];
        }
        image->SetSpacing(spacing.data());
        image->SetOrigin(origin.data());
        return;
    }

    assert(false);
}

// https://lists.osgeo.org/pipermail/gdal-dev/2011-September/030156.html
int utm_getZone(const double longitude)
{
    return static_cast<int>(1.0 + (longitude + 180.0) / 6.0);
}

bool utm_isNorthern(const double latitude)
{
    return latitude > 0.0;
}

class Proj4PJ
{
public:
    explicit Proj4PJ(const std::string & stringRepresentation)
        : m_stringRepr{ stringRepresentation }
        , m_pj{ nullptr }
    {
        m_pj = pj_init_plus(m_stringRepr.c_str());
        if (!m_pj)
        {
            m_errorString = pj_strerrno(pj_errno);
        }
    }

    ~Proj4PJ()
    {
        if (m_pj)
        {
            pj_free(m_pj);
        }
    }

    bool isValid() const
    {
        return m_pj != nullptr;
    }
    explicit operator bool() const
    {
        return isValid();
    }

    const std::string & errorString() const
    {
        return m_errorString;
    }

    vtkVector3d convertTo(const Proj4PJ & other, vtkVector3d coord, std::string * errorString = nullptr) const
    {
        if (pj_is_latlong(m_pj))
        {
            for (int i = 0; i < 3; ++i)
            {
                coord[i] = vtkMath::RadiansFromDegrees(coord[i]);
            }
        }

        double * rawData = coord.GetData();
        const int result = pj_transform(m_pj, other.m_pj, 1, 3,
            rawData, rawData + 1, rawData + 2);
        if (errorString)
        {
            if (result == 0)
            {
                errorString->clear();
            }
            else
            {
                *errorString = pj_strerrno(result);
            }
        }
        assert(result == 0);
        return coord;
    }

    void convertTo(const Proj4PJ & other, vtkDataArray & array, std::string * errorString = nullptr) const
    {
        VTK_ASSUME(array.GetNumberOfComponents() == 3);

        // NOTE/TODO: This could be implemented without a copy for vtkAOSDataArrayTemplate<double>
        // if required.
        auto doubleInArray = vtkDoubleArray::SafeDownCast(&array);
        const bool copyData = doubleInArray == nullptr;
        auto computeArray = copyData
            ? vtkSmartPointer<vtkDoubleArray>::New()
            : vtkSmartPointer<vtkDoubleArray>(doubleInArray);
        if (copyData)
        {
            computeArray->DeepCopy(&array);
        }

        if (pj_is_latlong(m_pj))
        {
            const vtkVector2d scaleToRadians(vtkMath::RadiansFromDegrees(1.0));
            scaleShiftArray(*computeArray, scaleToRadians, vtkVector2d(0.0));
        }

        const int result = pj_transform(m_pj, other.m_pj,
            computeArray->GetNumberOfTuples(), 3,
            computeArray->GetPointer(0),
            computeArray->GetPointer(1),
            computeArray->GetPointer(2));

        if (errorString)
        {
            if (result == 0)
            {
                errorString->clear();
            }
            else
            {
                *errorString = pj_strerrno(result);
            }
        }
        assert(result == 0);
        if (result != 0)
        {
            return;
        }

        if (pj_is_latlong(other.m_pj))
        {
            const vtkVector2d scaleToDegrees(vtkMath::DegreesFromRadians(1.0));
            scaleShiftArray(*computeArray, scaleToDegrees, vtkVector2d(0.0));
        }

        array.DeepCopy(computeArray);
    }

private:
    const std::string m_stringRepr;
    std::string m_errorString;
    projPJ m_pj;
};

template<unsigned int ModifyComponents>
void convertMetricHorizontalUnit(vtkDataSet & dataSet, const QString & inUnit, const QString & outUnit)
{
    const double scaleFactor = mathhelper::scaleFactorForMetricUnits(inUnit, outUnit);
    if (scaleFactor == 1.0)
    {
        return;
    }
    const vtkVector<double, ModifyComponents> scale(scaleFactor);

    scaleShiftDataSet(dataSet, scale, vtkVector<double, ModifyComponents>(0.0));
}

// Converts horizontal coordinate between different systems.
// It does not modify elevations.
std::string transformCoodinates(vtkDataSet & dataSet,
    const Proj4PJ & inSystem,
    const Proj4PJ & outSystem)
{
    std::string errorString;

    if (auto pointSet = vtkPointSet::SafeDownCast(&dataSet))
    {
        auto coords = pointSet->GetPoints()->GetData();
        inSystem.convertTo(outSystem, *coords, &errorString);
        return errorString;
    }

    if (auto image = vtkImageData::SafeDownCast(&dataSet))
    {
        ImageExtent extent;
        image->GetExtent(extent.data());
        if (extent.isEmpty())
        {
            return errorString;
        }
        DEBUG_ONLY(
        const double inOriginZ = image->GetOrigin()[2];
        const double inSpacingZ = image->GetSpacing()[2];
        );

        auto fixedPoints = vtkSmartPointer<vtkDoubleArray>::New();
        fixedPoints->SetNumberOfComponents(3);
        fixedPoints->SetNumberOfTuples(3);
        fixedPoints->SetTuple(0, image->GetOrigin());
        DataBounds bounds;
        image->GetBounds(bounds.data());
        const auto southWest = bounds.min();
        const auto northEast = bounds.max();
        fixedPoints->SetTuple(1, southWest.GetData());
        fixedPoints->SetTuple(2, northEast.GetData());
        inSystem.convertTo(outSystem, *fixedPoints, &errorString);
        if (!errorString.empty())
        {
            return errorString;
        }
        image->SetOrigin(fixedPoints->GetTuple(0));
        const auto pointsPerDimension = extent.convertTo<2>().componentSize() + 1;
        DataExtent<double, 2u> bounds2D;
        bounds2D.add(convertTo<2>(vtkVector3d(fixedPoints->GetTuple(1))));
        bounds2D.add(convertTo<2>(vtkVector3d(fixedPoints->GetTuple(2))));
        vtkVector3d spacing;
        image->GetSpacing(spacing.GetData());
        spacing[0] = pointsPerDimension[0] <= 1 ? spacing[0]
            : bounds2D.componentSize()[0] / (pointsPerDimension[0] - 1);
        spacing[1] = pointsPerDimension[1] <= 1 ? spacing[1]
            : bounds2D.componentSize()[1] / (pointsPerDimension[1] - 1);
        image->SetSpacing(spacing.GetData());
        assert(image->GetOrigin()[2] == inOriginZ);
        assert(image->GetSpacing()[2] == inSpacingZ);
        return errorString;
    }

    assert(false);
    return errorString;
}

}


vtkStandardNewMacro(GeographicTransformationFilter);


bool GeographicTransformationFilter::IsTransformationSupported(
    const ReferencedCoordinateSystemSpecification & sourceSpec,
    const CoordinateSystemSpecification & targetSpec)
{
    return sourceSpec.isValid() && targetSpec.isValid()
        && !sourceSpec.isUnspecified() && !targetSpec.isUnspecified()
        && sourceSpec.geographicSystem == targetSpec.geographicSystem
        && sourceSpec.globalMetricSystem == targetSpec.globalMetricSystem
        && sourceSpec.geographicSystem == "WGS 84"
        && sourceSpec.globalMetricSystem == "UTM";
}

GeographicTransformationFilter::GeographicTransformationFilter()
    : Superclass()
    , OperateInPlace{ false }
{
}

GeographicTransformationFilter::~GeographicTransformationFilter() = default;

int GeographicTransformationFilter::RequestInformation(
    vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto inData = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!inData)
    {
        return 0;
    }

    this->SourceCoordinateSystem =
        ReferencedCoordinateSystemSpecification::fromInformation(*inInfo);
    if (!this->SourceCoordinateSystem.isValid())
    {
        this->SourceCoordinateSystem =
            ReferencedCoordinateSystemSpecification::fromFieldData(*inData->GetFieldData());
    }
    if (!this->SourceCoordinateSystem.isValid())
    {
        vtkErrorMacro("Missing input coordinate system information.");
        return 0;
    }

    if (!IsTransformationSupported(this->SourceCoordinateSystem, this->TargetCoordinateSystem))
    {
        vtkErrorMacro(<< "Coordinate system conversion not supported: " << vtkOStreamWrapper::EndlType()
            << "From " << this->SourceCoordinateSystem << std::endl
            << "To " << this->TargetCoordinateSystem << std::endl);
        return 0;
    }

    const auto & sourceUnit = this->SourceCoordinateSystem.unitOfMeasurement;
    const auto & targetUnit = this->TargetCoordinateSystem.unitOfMeasurement;

    switch (this->SourceCoordinateSystem.type.value)
    {
    case CoordinateSystemType::metricLocal:
    case CoordinateSystemType::metricGlobal:
        if (!mathhelper::isValidMetricUnit(sourceUnit))
        {
            vtkErrorMacro(<< "Invalid source metric unit: " << sourceUnit.toStdString());
            return 0;
        }
        break;
    default:
        break;
    }

    switch (this->TargetCoordinateSystem.type.value)
    {
    case CoordinateSystemType::metricLocal:
    case CoordinateSystemType::metricGlobal:
        if (!mathhelper::isValidMetricUnit(targetUnit))
        {
            vtkErrorMacro(<< "Invalid target metric unit: " << targetUnit.toStdString());
            return 0;
        }
        break;
    default:
        break;
    }

    this->ReferencedTargetSpec =
        ReferencedCoordinateSystemSpecification(this->TargetCoordinateSystem,
            this->SourceCoordinateSystem.referencePointLatLong);

    auto outInfo = outputVector->GetInformationObject(0);
    this->ReferencedTargetSpec.writeToInformation(*outInfo);

    // Remove information keys that are invalidated by this filter.
    // For vtkImageData inputs, these values could already be computed, actually.
    if (outInfo->Has(vtkDataObject::BOUNDING_BOX()))
    {
        outInfo->Remove(vtkDataObject::BOUNDING_BOX());
    }
    if (outInfo->Has(vtkDataObject::SPACING()))
    {
        outInfo->Remove(vtkDataObject::SPACING());
    }
    if (outInfo->Has(vtkDataObject::ORIGIN()))
    {
        outInfo->Remove(vtkDataObject::ORIGIN());
    }

    return 1;
}

int GeographicTransformationFilter::RequestData(
    vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * /*outputVector*/)
{
    auto input = vtkDataSet::SafeDownCast(GetInput());
    auto output = vtkDataSet::SafeDownCast(GetOutput());

    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    this->ReferencedTargetSpec.writeToFieldData(*output->GetFieldData());

    const auto & sourceUnit = this->SourceCoordinateSystem.unitOfMeasurement;
    const auto & targetUnit = this->TargetCoordinateSystem.unitOfMeasurement;
    const CoordinateSystemType sourceType = this->SourceCoordinateSystem.type;
    const CoordinateSystemType targetType = this->TargetCoordinateSystem.type;

    // Catch the simplest cases: no system transformation
    if (sourceType == targetType)
    {
        if (sourceType == CoordinateSystemType::geographic)
        {
            // Nothing to do!
            return 1;
        }
    }

    // Copy coordinate array, so that we don't overwrite the input.
    // (proj.4 operates in-place)
    if (!this->OperateInPlace)
    {
        if (auto inputPointSet = vtkPointSet::SafeDownCast(input))
        {
            assert(vtkPointSet::SafeDownCast(output));
            auto outputPointSet = static_cast<vtkPointSet *>(output);
            auto outputPoints = vtkSmartPointer<vtkPoints>::New();
            outputPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
            outputPoints->DeepCopy(inputPointSet->GetPoints());
            outputPointSet->SetPoints(outputPoints);
        }
    }

    // A further simple case: metric unit transformation only.
    if (sourceType == targetType)
    {
        assert(sourceType == CoordinateSystemType::metricLocal
            || sourceType == CoordinateSystemType::metricGlobal);
        // Also scale the elevations to the target unit.
        convertMetricHorizontalUnit<3>(*output, sourceUnit, targetUnit);
        return 1;
    }

    // Starting from here, a valid reference point is required.
    assert(this->SourceCoordinateSystem.isReferencePointValid());
    if (!this->SourceCoordinateSystem.isReferencePointValid())
    {
        return 0;
    }

    const vtkVector2d & refLatLongWGS84 = this->SourceCoordinateSystem.referencePointLatLong;

    // http://spatialreference.org/ref/epsg/wgs-84/
    static const Proj4PJ pj_longlat_WGS84{ "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs" };
    if (!pj_longlat_WGS84)
    {
        vtkErrorMacro(<< "Invalid geographic coordinate system specified for PROJ.4: "
            << pj_longlat_WGS84.errorString());
        return 0;
    }
    const int utmZone = utm_getZone(refLatLongWGS84.GetY());
    const Proj4PJ pj_UTM{ "+proj=utm +zone=" + std::to_string(utmZone)
        + (utm_isNorthern(refLatLongWGS84.GetX()) ? " " : " +south ")
        + "+ellps=WGS84 +datum=WGS84 +units=m +no_defs" };
    if (!pj_UTM)
    {
        vtkErrorMacro(<< "Invalid projected coordinate system specified for PROJ.4: "
            << pj_UTM.errorString());
        return 0;
    }

    std::string errorString;

    // Acquire reference point in UTM (in meters)
    const vtkVector2d refUTM_m = convertTo<2>(pj_longlat_WGS84.convertTo(
        pj_UTM, { refLatLongWGS84.GetY(), refLatLongWGS84.GetX(), 0.0 }, &errorString));

    if (!errorString.empty())
    {
        vtkErrorMacro(<< "PROJ.4 transformation failed: " << errorString);
        return 0;
    }

    // Projected/metric local<->global transformation -> shift by metric reference point.
    // (This does not change to absolute spatial size of the data set, except for unit changes).
    if (sourceType != CoordinateSystemType::geographic
        && targetType != CoordinateSystemType::geographic)
    {
        const double sourceUnitScale = mathhelper::scaleFactorForMetricUnits(sourceUnit, "m");
        const double targetUnitScale = mathhelper::scaleFactorForMetricUnits("m", targetUnit);
        const double unitScale = sourceUnitScale * targetUnitScale;

        // Shift local -> global metric coordinates
        // Apply the shift in target units (not necessarily in meters).
        auto shiftInTargetUnit = refUTM_m * targetUnitScale;

        if (sourceType == CoordinateSystemType::metricGlobal
            && targetType == CoordinateSystemType::metricLocal)
        {
            // ...or invert, global -> local
            shiftInTargetUnit *= -1.0;
        }
        else
        {
            assert(sourceType == CoordinateSystemType::metricLocal
                && targetType == CoordinateSystemType::metricGlobal);
        }
        
        scaleShiftDataSet(*output,
            vtkVector3d{ unitScale },   // scale from source to target unit
            // shift from/to global/local reference point
            vtkVector3d{ shiftInTargetUnit[0], shiftInTargetUnit[1], 0.0 });
        // Done!
        return 1;
    }


    // In code below here, elevations are just passed through.

    assert(sourceType == CoordinateSystemType::geographic
        || targetType == CoordinateSystemType::geographic);

    // Transform from global to global metric, and local metric if requested.
    if (sourceType == CoordinateSystemType::geographic)
    {
        assert(targetType != CoordinateSystemType::geographic);
        errorString = transformCoodinates(*output, pj_longlat_WGS84, pj_UTM);
        if (!errorString.empty())
        {
            vtkErrorMacro(<< "Coordinate transformation failed: " << errorString);
            return 0;
        }

        // PROJ.4 converts to global coordinates, so convert to local if needed.
        // Also adjust to unit if required.
        vtkVector2d shift{ 0.0, 0.0 };
        if (targetType == CoordinateSystemType::metricLocal)
        {
            shift = -refUTM_m;
        }
        // First scale, than shift. So scale the shift if required.
        const double targetUnitScale = mathhelper::scaleFactorForMetricUnits("m", targetUnit);
        const auto scale = vtkVector2d(targetUnitScale, targetUnitScale);
        shift *= targetUnitScale;
        scaleShiftDataSet(*output, scale, shift);
    }

    // Transform some metric unit to geographic coordinates.
    else
    {
        assert(targetType == CoordinateSystemType::geographic);
        // Conversion to geographic coordinates! Make sure source is in global coordinates, in meters.
        if (targetType == CoordinateSystemType::geographic
            && sourceType == CoordinateSystemType::metricLocal)
        {
            const double sourceUnitScale = mathhelper::scaleFactorForMetricUnits(sourceUnit, "m");

            scaleShiftDataSet(*output,
                vtkVector2d{ sourceUnitScale, sourceUnitScale },  // Convert to meters.
                refUTM_m); // Shift the previous (0,0) to the global reference point.
        }

        errorString = transformCoodinates(*output, pj_UTM, pj_longlat_WGS84);
        if (!errorString.empty())
        {
            vtkErrorMacro(<< "Coordinate transformation failed: " << errorString);
            return 0;
        }
    }

    return 1;
}

int GeographicTransformationFilter::FillInputPortInformation(int /*port*/, vtkInformation * info)
{
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    return 1;
}
