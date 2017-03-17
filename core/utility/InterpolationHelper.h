#pragma once

#include <core/core_api.h>


class QString;
class vtkDataArray;
class vtkDataSet;
class vtkImageData;
class vtkPolyData;
template<typename T> class vtkSmartPointer;


class CORE_API InterpolationHelper
{
public:
    static vtkSmartPointer<vtkDataArray> interpolate(
        vtkDataSet & baseDataSet, 
        vtkDataSet & sourceDataSet,
        const QString & sourceAttributeName, 
        bool attributeInCellData,
        bool targetAttributeInCellData);

    // Fast image on image interpolation for images with matching structure
    static vtkSmartPointer<vtkDataArray> fastInterpolateImageOnImage(
        vtkImageData & baseImage,
        vtkImageData & sourceImage,
        const QString & sourceAttributeName);

    // Fast poly on poly interpolation for data sets with matching structure
    static vtkSmartPointer<vtkDataArray> fastInterpolatePolyOnPoly(
        vtkPolyData & basePoly,
        vtkPolyData & sourcePoly,
        const QString & sourceAttributeName,
        bool attributeInCellData);
};
