#include "datagenerator.h"

//#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkDelaunay2D.h>


#include <vtkPolyDataMapper.h>


vtkSmartPointer<vtkPolyDataMapper> DataGenerator::generate(uint32_t gridSize, float xzScale, float yScale) const
{
    assert(gridSize > 0);
    assert(xzScale > 0);
    assert(yScale > 0);

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    int32_t offset = static_cast<int32_t>(gridSize) -static_cast<int32_t>((gridSize + 1) * 0.5);
    int32_t offsetSq = offset * offset;

    for (uint32_t a = 0; a < gridSize; ++a)
    for (uint32_t b = 0; b < gridSize; ++b)
    {
        double x = int32_t(a) - offset;
        double z = int32_t(b) - offset;
        double y = offsetSq - (x*x + z*z);
        std::cout << "Add: " << x << ":" << y << ":" << z << std::endl;
        points->InsertNextPoint(x * xzScale, y * yScale, z * xzScale);
    }

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);

    vtkSmartPointer<vtkDelaunay2D> delauney2D = vtkSmartPointer<vtkDelaunay2D>::New();
    delauney2D->SetInputData(polyData);
    delauney2D->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(delauney2D->GetOutputPort());

    return mapper;
}