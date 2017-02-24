#include <core/utility/InterpolationHelper.h>

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
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkSmartPointer<vtkDataArray> InterpolationHelper::interpolateImageOnImage(
    vtkImageData & baseImage,
    vtkImageData & sourceImage,
    const QString & sourceAttributeName)
{
    // Fast image on image interpolation for images with matching structure

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

    if (sourceAttributeName.isEmpty())
    {
        return sourceImage.GetPointData()->GetScalars();
    }

    return sourceImage.GetPointData()->GetArray(sourceAttributeName.toUtf8().data());
}


/** Interpolate the source's attributes to the structure of the base data set */
vtkSmartPointer<vtkDataArray> InterpolationHelper::interpolate(vtkDataSet & baseDataSet, vtkDataSet & sourceDataSet, const QString & sourceAttributeName, bool attributeInCellData)
{
    auto baseImage = vtkImageData::SafeDownCast(&baseDataSet);
    auto sourceImage = vtkImageData::SafeDownCast(&sourceDataSet);

    if (baseImage && sourceImage)
    {
        // just pass the data for structurally matching images
        auto result = interpolateImageOnImage(*baseImage, *sourceImage, sourceAttributeName);
        if (result)
        {
            return result;
        }
    }

    auto basePoly = vtkPolyData::SafeDownCast(&baseDataSet);
    auto sourcePoly = vtkPolyData::SafeDownCast(&sourceDataSet);

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
            attributeInCellData ? vtkAssignAttribute::CELL_DATA : vtkAssignAttribute::POINT_DATA);
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

    // the probe creates point based values for polygons, but we want them associated with the centroids
    if (basePoly)
    {
        auto pointToCellData = vtkSmartPointer<vtkPointDataToCellData>::New();
        pointToCellData->SetInputConnection(probe->GetOutputPort());
        resultAlgorithm = pointToCellData;
    }

    resultAlgorithm->Update();

    auto probedDataSet = vtkDataSet::SafeDownCast(resultAlgorithm->GetOutputDataObject(0));

    if (basePoly)
    {
        if (sourceAttributeName.isEmpty())
        {
            return probedDataSet->GetCellData()->GetScalars();
        }
        return probedDataSet->GetCellData()->GetArray(sourceAttributeName.toUtf8().data());
    }

    if (sourceAttributeName.isEmpty())
    {
        return probedDataSet->GetPointData()->GetScalars();
    }
    return probedDataSet->GetPointData()->GetArray(sourceAttributeName.toUtf8().data());
}
