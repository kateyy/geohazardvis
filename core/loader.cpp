#include "loader.h"

#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <vtkCellArray.h>

#include "common/file_parser.h"

std::shared_ptr<Input> Loader::loadFileAsPoints(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake)
{
    std::shared_ptr<Input> input = std::make_shared<Input>();
    input->polyData = loadData(filename, nbColumns, firstToTake);

    input->dataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    input->dataMapper->SetInputData(input->polyData);

    return input;
}

std::shared_ptr<Input> Loader::loadFileTriangulated(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake)
{
    std::shared_ptr<Input> input = std::make_shared<Input>();
    input->polyData = loadData(filename, nbColumns, firstToTake);

    // create triangles from vertex list
    input->algorithm = vtkSmartPointer<vtkDelaunay2D>::New();
    input->algorithm->SetInputData(input->polyData);
    input->algorithm->Update();

    input->dataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    input->dataMapper->SetInputConnection(input->algorithm->GetOutputPort());

    return input;
}

vtkSmartPointer<vtkPolyData> Loader::loadData(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake)
{
    assert(nbColumns >= 3 + firstToTake);

    std::vector<std::vector<t_FP>> parsedData;
    parsedData.resize(nbColumns);

    // load input file
    populateIOVectors(filename, parsedData);

    assert(parsedData.size() == nbColumns);

    size_t nbRows = parsedData.at(0).size();
    assert(nbRows > 0);

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    vtkIdType * pid = new vtkIdType[nbRows];

    // copy triangle vertices to vtk point list
    for (size_t row = 0; row < nbRows; ++row) {
        pid[row] = points->InsertNextPoint(parsedData[firstToTake][row], parsedData[firstToTake + 1][row], parsedData[firstToTake + 2][row]);
    }

    // Create the topology of the point (a vertex)
    vtkSmartPointer<vtkCellArray> vertices =
        vtkSmartPointer<vtkCellArray>::New();

    vertices->InsertNextCell(nbRows, pid);

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetVerts(vertices);

    return polyData;
}

vtkSmartPointer<vtkActor> Input::createActor()
{
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(dataMapper);
    actor->GetProperty()->SetRepresentationToWireframe();

    return actor;
}
