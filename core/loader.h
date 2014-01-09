#pragma once

#include <vtkSmartPointer.h>

#include "common/ebem3d_common.h"

class vtkPolyDataMapper;
class vtkPolyData;

class Loader
{
public:
    static vtkSmartPointer<vtkPolyDataMapper> loadFileAsPoints(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);
    static vtkSmartPointer<vtkPolyDataMapper> loadFileTriangulated(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);

protected:
    static vtkSmartPointer<vtkPolyData> loadData(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);
};
