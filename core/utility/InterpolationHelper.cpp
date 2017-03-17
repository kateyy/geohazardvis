#include <core/utility/InterpolationHelper.h>

#include <algorithm>

#include <QString>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTrivialProducer.h>
#include <vtkVector.h>

#include <core/filters/ImageBlankNonFiniteValuesFilter.h>
#include <core/filters/SetMaskedPointScalarsToNaNFilter.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{

vtkDataArray * extractAttribute(const QString & name, bool attributeInCellData, vtkDataSet & dataSet)
{
    auto & attributes = attributeInCellData
        ? static_cast<vtkDataSetAttributes &>(*dataSet.GetCellData())
        : static_cast<vtkDataSetAttributes &>(*dataSet.GetPointData());

    if (name.isEmpty())
    {
        return attributes.GetScalars();
    }
    return attributes.GetArray(name.toUtf8().data());
};

}

vtkSmartPointer<vtkDataArray> InterpolationHelper::fastInterpolateImageOnImage(
    vtkImageData & baseImage,
    vtkImageData & sourceImage,
    const QString & sourceAttributeName)
{
    ImageExtent baseExtent(baseImage.GetExtent()), sourceExtent(sourceImage.GetExtent());
    vtkVector3d baseOrigin(baseImage.GetOrigin()), sourceOrigin(sourceImage.GetOrigin());
    vtkVector3d baseSpacing(baseImage.GetSpacing()), sourceSpacing(sourceImage.GetSpacing());

    // Check if the images have the same structure (all points are overlapping).
    // If the image is just plane and not a volume, don't check the flat dimension.
    if (baseImage.GetDataDimension() == 2)
    {
        unsigned flatDimension = 3;
        for (unsigned i = 0; i < 3; ++i)
        {
            if (baseExtent.componentSize()[i] == 0
                && sourceExtent.componentSize()[i] == 0)
            {
                flatDimension = i;
                break;
            }
        }
        if (flatDimension <= 2)
        {
            baseExtent.setDimension(flatDimension, 0, 0);
            sourceExtent.setDimension(flatDimension, 0, 0);
            baseOrigin[flatDimension] = sourceOrigin[flatDimension] = 0.0;
            baseSpacing[flatDimension] = sourceSpacing[flatDimension] = 1.0;
        }
    }

    if (baseExtent != sourceExtent)
    {
        return nullptr;
    }

    if (baseOrigin != sourceOrigin || baseSpacing != baseSpacing)
    {
        const double epsilon = 10.e-5;
        const auto o = maxComponent(abs(baseOrigin - sourceOrigin));
        const auto s = maxComponent(abs(baseSpacing - sourceSpacing));
        if (o > epsilon || s > epsilon)
        {
            return nullptr;
        }
    }

    return extractAttribute(sourceAttributeName, false, sourceImage);
}

vtkSmartPointer<vtkDataArray> InterpolationHelper::fastInterpolatePolyOnPoly(
    vtkPolyData & basePoly,
    vtkPolyData & sourcePoly,
    const QString & sourceAttributeName,
    bool attributeInCellData)
{
    if (// Point coordinates are required.
        basePoly.GetNumberOfPoints() != sourcePoly.GetNumberOfPoints()
        // Lines / strips are not supported here
        || basePoly.GetNumberOfLines() != 0 || sourcePoly.GetNumberOfLines() != 0
        || basePoly.GetNumberOfStrips() != 0 || sourcePoly.GetNumberOfStrips() != 0
        // Number of verts/polys must match and only one type at a time is supported
        || basePoly.GetNumberOfVerts() != sourcePoly.GetNumberOfVerts()
        || basePoly.GetNumberOfPolys() != sourcePoly.GetNumberOfPolys()
        || (basePoly.GetNumberOfPolys() > 0 && basePoly.GetNumberOfVerts() > 0))
    {
        return{};
    }


    // First: cheep index comparison
    const bool isPointCloud = basePoly.GetNumberOfVerts() > 0;
    auto & baseCells = isPointCloud ? *basePoly.GetVerts() : *basePoly.GetPolys();
    auto & sourceCells = isPointCloud ? *sourcePoly.GetVerts() : *sourcePoly.GetPolys();
    baseCells.InitTraversal();
    sourceCells.InitTraversal();
    while (true)
    {
        vtkIdType baseNumCellPoints, sourceNumCellPoints;
        vtkIdType * baseCellPointIds, *sourceCellPointIds;
        const auto baseHasNextCell = baseCells.GetNextCell(baseNumCellPoints, baseCellPointIds);
        const auto sourceHasNextCell = sourceCells.GetNextCell(sourceNumCellPoints, sourceCellPointIds);
        if (baseHasNextCell != sourceHasNextCell
            || baseNumCellPoints != sourceNumCellPoints)
        {
            return{};
        }
        if (!baseHasNextCell)
        {
            // okay, all cells are equal
            break;
        }
        assert(baseCellPointIds && sourceCellPointIds);
        const auto baseEndIt = baseCellPointIds + baseNumCellPoints;
        const auto its = std::mismatch(baseCellPointIds, baseEndIt, sourceCellPointIds);
        if (its.first != baseEndIt)
        {
            return{};
        }
    }

    const auto numPoints = basePoly.GetNumberOfPoints();
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        vtkVector3d basePoint, sourcePoint;
        basePoly.GetPoint(i, basePoint.GetData());
        sourcePoly.GetPoint(i, sourcePoint.GetData());

        const double epsilon = 10.e-5;
        // Check if the individual components are close enough, don't compute expensive root.
        if (maxComponent(abs(basePoint - sourcePoint)) > epsilon)
        {
            return{};
        }

    }

    return extractAttribute(sourceAttributeName, attributeInCellData, sourcePoly);
}


/** Interpolate the source's attributes to the structure of the base data set */
vtkSmartPointer<vtkDataArray> InterpolationHelper::interpolate(
    vtkDataSet & baseDataSet,
    vtkDataSet & sourceDataSet,
    const QString & sourceAttributeName,
    bool sourceAttributeInCellData,
    bool targetAttributeInCellData)
{
    if (&baseDataSet == &sourceDataSet)
    {
        return extractAttribute(sourceAttributeName, sourceAttributeInCellData, sourceDataSet);
    }

    auto baseImage = vtkImageData::SafeDownCast(&baseDataSet);
    auto sourceImage = vtkImageData::SafeDownCast(&sourceDataSet);

    if (baseImage && sourceImage)
    {
        // just pass the data for structurally matching images
        auto result = fastInterpolateImageOnImage(*baseImage, *sourceImage, sourceAttributeName);
        if (result)
        {
            return result;
        }
    }

    auto basePoly = vtkPolyData::SafeDownCast(&baseDataSet);
    auto sourcePoly = vtkPolyData::SafeDownCast(&sourceDataSet);

    if (basePoly && sourcePoly)
    {
        auto result = fastInterpolatePolyOnPoly(*basePoly, *sourcePoly, sourceAttributeName, sourceAttributeInCellData);
        if (result)
        {
            return result;
        }
    }

    auto baseDataProducer = vtkSmartPointer<vtkTrivialProducer>::New();
    baseDataProducer->SetOutput(&baseDataSet);
    auto sourceDataProducer = vtkSmartPointer<vtkTrivialProducer>::New();
    sourceDataProducer->SetOutput(&sourceDataSet);

    vtkSmartPointer<vtkAlgorithm> baseDataFilter = baseDataProducer;
    vtkSmartPointer<vtkAlgorithm> sourceDataFilter = sourceDataProducer;

    // assign source data scalars, if required
    if (!sourceAttributeName.isEmpty())
    {
        auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
        assign->Assign(sourceAttributeName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
            sourceAttributeInCellData ? vtkAssignAttribute::CELL_DATA : vtkAssignAttribute::POINT_DATA);
        assign->SetInputConnection(sourceDataFilter->GetOutputPort());
        sourceDataFilter = assign;
    }

    // flatten polygonal data before interpolation

    auto createFlattener = [](vtkAlgorithm * upstream) -> vtkSmartPointer<vtkAlgorithm>
    {
        auto flattenerTransform = vtkSmartPointer<vtkTransform>::New();
        flattenerTransform->Scale(1, 1, 0);
        auto flattener = vtkSmartPointer<vtkTransformFilter>::New();
        flattener->SetTransform(flattenerTransform);
        flattener->SetInputConnection(upstream->GetOutputPort());

        return flattener;
    };

    if (basePoly)
    {
        baseDataFilter = createFlattener(baseDataFilter);
    }
    if (sourcePoly)
    {
        sourceDataFilter = createFlattener(sourceDataFilter);
    }
    else if (sourceImage)
    {
        auto blankNaNs = vtkSmartPointer<ImageBlankNonFiniteValuesFilter>::New();
        blankNaNs->SetInputConnection(sourceDataFilter->GetOutputPort());
        sourceDataFilter = blankNaNs;
    }


    // now probe: at least one of the data sets is a polygonal one here

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(baseDataFilter->GetOutputPort());
    probe->SetSourceConnection(sourceDataFilter->GetOutputPort());

    vtkSmartPointer<vtkAlgorithm> resultAlgorithm = probe;

    // For polygons: vtkPobeFilter creates point based values but values per centroid are required
    if (basePoly && targetAttributeInCellData)
    {
        auto pointToCellData = vtkSmartPointer<vtkPointDataToCellData>::New();
        pointToCellData->SetInputConnection(probe->GetOutputPort());
        resultAlgorithm = pointToCellData;
    }
    if (baseImage && sourceImage)
    {
        auto maskedToNaN = vtkSmartPointer<SetMaskedPointScalarsToNaNFilter>::New();
        maskedToNaN->SetInputConnection(resultAlgorithm->GetOutputPort());
        resultAlgorithm = maskedToNaN;
    }

    resultAlgorithm->Update();

    auto probedDataSet = vtkDataSet::SafeDownCast(resultAlgorithm->GetOutputDataObject(0));
    if (!probedDataSet)
    {
        return{};
    }

    return extractAttribute(sourceAttributeName, targetAttributeInCellData, *probedDataSet);
}
