#pragma once

#include <core/core_api.h>


class QString;
class vtkDataArray;
class vtkDataSet;
class vtkImageData;
template<typename T> class vtkSmartPointer;


class CORE_API InterpolationHelper
{
public:
    static vtkSmartPointer<vtkDataArray> interpolateImageOnImage(
        vtkImageData & baseImage, 
        vtkImageData & sourceImage, 
        const QString & sourceAttributeName);

    static vtkSmartPointer<vtkDataArray> interpolate(
        vtkDataSet & baseDataSet, 
        vtkDataSet & sourceDataSet,
        const QString & sourceAttributeName, 
        bool attributeInCellData);
};
