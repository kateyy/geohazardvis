/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/utility/InterpolationHelper.h>

#include <algorithm>
#include <limits>

#include <QString>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkPointInterpolator.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTrivialProducer.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/filters/ImageBlankNonFiniteValuesFilter.h>
#include <core/filters/SetMaskedPointScalarsToNaNFilter.h>
#include <core/utility/DataExtent.h>
#include <core/utility/mathhelper.h>
#include <core/utility/types_utils.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{

vtkDataArray * extractAttribute(const QString & name, const IndexType location, vtkDataSet & dataSet)
{
    auto attributes = IndexType_util(location).extractAttributes(dataSet);
    if (!attributes)
    {
        return nullptr;
    }

    if (name.isEmpty())
    {
        return attributes->GetScalars();
    }
    return attributes->GetArray(name.toUtf8().data());
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

    return extractAttribute(sourceAttributeName, IndexType::points, sourceImage);
}

vtkSmartPointer<vtkDataArray> InterpolationHelper::fastInterpolatePolyOnPoly(
    vtkPolyData & basePoly,
    vtkPolyData & sourcePoly,
    const QString & sourceAttributeName,
    IndexType location)
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

    // TODO This should be exposed to the class interface/or even the UI
    // Default maximum distance if units of the data sets are not known
    double epsilon = 1.e-4;

    // Consider the units of measurement, if they are known for both data sets.
    const auto baseUnit = CoordinateSystemSpecification::fromFieldData(*basePoly.GetFieldData()).unitOfMeasurement;
    const auto sourceUnit = CoordinateSystemSpecification::fromFieldData(*sourcePoly.GetFieldData()).unitOfMeasurement;
    if (!baseUnit.isEmpty() && !sourceUnit.isEmpty())
    {
        if (QString(baseUnit) != QString(sourceUnit)) // different unit -> fail
        {
            return {};
        }
        if (mathhelper::isValidMetricUnit(baseUnit))
        {   // TODO This should be exposed to the class interface/or even the UI
            // Allow 10 cm derivation for metric coordinates
            epsilon = mathhelper::scaleFactorForMetricUnits("dm", baseUnit);
        }
    }

    const auto numPoints = basePoly.GetNumberOfPoints();
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        vtkVector3d basePoint, sourcePoint;
        basePoly.GetPoint(i, basePoint.GetData());
        sourcePoly.GetPoint(i, sourcePoint.GetData());

        // Check if the individual components are close enough, don't compute expensive root.
        if (maxComponent(abs(basePoint - sourcePoint)) > epsilon)
        {
            return{};
        }

    }

    return extractAttribute(sourceAttributeName, location, sourcePoly);
}


/** Interpolate the source's attributes to the structure of the base data set */
vtkSmartPointer<vtkDataArray> InterpolationHelper::interpolate(
    vtkDataSet & baseDataSet,
    vtkDataSet & sourceDataSet,
    const QString & sourceAttributeName,
    IndexType sourceLocation,
    IndexType targetLocation)
{
    if (&baseDataSet == &sourceDataSet)
    {
        if (sourceLocation == targetLocation)
        {
            return extractAttribute(sourceAttributeName, sourceLocation, sourceDataSet);
        }
        return nullptr;
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

    if (basePoly && sourcePoly && (sourceLocation == targetLocation))
    {
        auto result = fastInterpolatePolyOnPoly(
            *basePoly, *sourcePoly, sourceAttributeName, sourceLocation);
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
            IndexType_util(sourceLocation).toVtkAssignAttribute_AttributeLocation());
        assign->SetInputConnection(sourceDataFilter->GetOutputPort());
        sourceDataFilter = assign;
    }

    // flatten polygonal data before interpolation

    auto createFlattener = [] (vtkAlgorithm * upstream) -> vtkSmartPointer<vtkAlgorithm>
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

    vtkSmartPointer<vtkAlgorithm> probe;
    // Interpolation between point base data set?
    // vtkProbeFilter always tries to find closest cells, which is quite bad for point clouds
    // that only define one cell containing all points.
    // vtkPointInterpolator interpolates on point coordinates only.
    if (sourcePoly
        && sourcePoly->GetNumberOfVerts() == sourcePoly->GetNumberOfCells()
        && (sourceImage
            || (sourcePoly
                && sourcePoly->GetNumberOfVerts() == sourcePoly->GetNumberOfCells())))

    {
        auto pointProbe = vtkSmartPointer<vtkPointInterpolator>::New();
        pointProbe->SetNullPointsStrategyToNullValue();
        pointProbe->SetNullValue(std::numeric_limits<double>::quiet_NaN());
        pointProbe->SetInputConnection(baseDataFilter->GetOutputPort());
        pointProbe->SetSourceConnection(sourceDataFilter->GetOutputPort());
        probe = pointProbe;
    }
    else
    {
        auto cellProbe = vtkSmartPointer<vtkProbeFilter>::New();
        cellProbe->SetInputConnection(baseDataFilter->GetOutputPort());
        cellProbe->SetSourceConnection(sourceDataFilter->GetOutputPort());
        probe = cellProbe;
    }
    assert(probe);

    vtkSmartPointer<vtkAlgorithm> resultAlgorithm = probe;

    // For polygons: vtkPobeFilter creates point based values but values per centroid are required
    if (basePoly && (targetLocation == IndexType::cells))
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

    if (!resultAlgorithm->GetExecutive()->Update())
    {
        return{};
    }

    auto probedDataSet = vtkDataSet::SafeDownCast(resultAlgorithm->GetOutputDataObject(0));
    if (!probedDataSet)
    {
        return{};
    }

    return extractAttribute(sourceAttributeName, targetLocation, *probedDataSet);
}
