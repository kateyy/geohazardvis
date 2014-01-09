#pragma once

#include <memory>

#include <vtkSmartPointer.h>

#include "common/ebem3d_common.h"

class vtkPolyDataMapper;
class vtkPolyData;
class vtkActor;
class vtkPolyDataAlgorithm;


struct Input {
    vtkSmartPointer<vtkPolyData> polyData;
    vtkSmartPointer<vtkPolyDataAlgorithm> algorithm;
    vtkSmartPointer<vtkPolyDataMapper> dataMapper;

    vtkSmartPointer<vtkActor> createActor();
};

class Loader
{
public:
    static std::shared_ptr<Input> loadFileAsPoints(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);
    static std::shared_ptr<Input> loadFileTriangulated(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);

protected:
    static vtkSmartPointer<vtkPolyData> loadData(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);
};
