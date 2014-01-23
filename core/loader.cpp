#include "loader.h"

#include <cmath>

#include <vtkPoints.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkProperty.h>
#include <vtkCellArray.h>

#include "input.h"
#include "common/file_parser.h"

std::shared_ptr<Input> Loader::loadFileAsPoints(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake)
{
    std::shared_ptr<Input> input = std::make_shared<Input>(filename);
    ParsedData * parsedData = loadData(filename, nbColumns);
    input->setPolyData(parsePoints(*parsedData, firstToTake));
    delete parsedData;

    return input;
}

std::shared_ptr<ProcessedInput> Loader::loadFileTriangulated(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake)
{
    std::shared_ptr<ProcessedInput> input = std::make_shared<ProcessedInput>(filename);
    ParsedData * parsedData = loadData(filename, nbColumns);
    input->setPolyData(parsePoints(*parsedData, firstToTake));
    delete parsedData;

    // create triangles from vertex list
    input->algorithm = vtkSmartPointer<vtkDelaunay2D>::New();
    input->algorithm->SetInputData(input->polyData());
    input->algorithm->Update();

    return input;
}

std::shared_ptr<Input> Loader::loadIndexedTriangles(
    const std::string & vertexFilename, t_UInt nbVertexFileColumns, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const std::string & indexFilename, t_UInt nbIndexFileColumns, t_UInt firstIndexColumn)
{
    std::shared_ptr<Input> input = std::make_shared<Input>(vertexFilename);
    ParsedData * vertexData = loadData(vertexFilename, nbVertexFileColumns);
    ParsedData * indexData = loadData(indexFilename, nbIndexFileColumns);

    input->setPolyData(parseIndexedTriangles(
        *vertexData, vertexIndexColumn, firstVertexColumn,
        *indexData, firstIndexColumn));

    return input;
}

Loader::ParsedData * Loader::loadData(const std::string & filename, t_UInt nbColumns)
{
    assert(nbColumns > 0);

    Loader::ParsedData * parsedData = new ParsedData();
    parsedData->resize(nbColumns);

    // load input file
    populateIOVectors(filename, *parsedData);

    assert(parsedData->size() == nbColumns);

    assert(parsedData->at(0).size() > 0);

    return parsedData;
}

vtkSmartPointer<vtkPolyData> Loader::parsePoints(const ParsedData & parsedData, t_UInt firstColumn)
{
    assert(parsedData.size() > firstColumn);

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    size_t nbRows = parsedData.at(firstColumn).size();
    std::vector<vtkIdType> pointIds(nbRows);

    // copy triangle vertices to vtk point list
    for (size_t row = 0; row < nbRows; ++row) {
        pointIds.at(row) = points->InsertNextPoint(parsedData[firstColumn][row], parsedData[firstColumn + 1][row], parsedData[firstColumn + 2][row]);
    }

    // Create the topology of the point (a vertex)
    vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();

    vertices->InsertNextCell(nbRows, pointIds.data());

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetVerts(vertices);

    return polyData;
}

vtkSmartPointer<vtkPolyData> Loader::parseIndexedTriangles(
    const ParsedData & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const ParsedData & parsedIndexData, t_UInt firstIndexColumn)
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    size_t nbVertices = parsedVertexData[vertexIndexColumn].size();
    size_t nbTriangles = parsedIndexData[firstIndexColumn].size();

    std::vector<vtkIdType> pointIds(nbVertices);

    // to let the internal indices start with 0
    t_UInt indexOffset = std::llround(parsedVertexData[vertexIndexColumn][0]);

    for (size_t row = 0; row < nbVertices; ++row) {
        points->InsertNextPoint(parsedVertexData[firstVertexColumn][row], parsedVertexData[firstVertexColumn + 1][row], parsedVertexData[firstVertexColumn + 2][row]);
        pointIds.at(row) = std::llround(parsedVertexData[vertexIndexColumn][row]) - indexOffset;
    }

    vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
    for (size_t row = 0; row < nbTriangles; ++row) {
        // set the indices for the three triangle vertices
        // hopefully, these indices map to the vertex indices from parsedVertexData
        triangle->GetPointIds()->SetId(0, std::llround(parsedIndexData[firstIndexColumn][row]) - indexOffset);
        triangle->GetPointIds()->SetId(1, std::llround(parsedIndexData[firstIndexColumn+1][row]) - indexOffset);
        triangle->GetPointIds()->SetId(2, std::llround(parsedIndexData[firstIndexColumn+2][row]) - indexOffset);
        triangles->InsertNextCell(triangle);    // this copies the triangle data into the list
    }

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetPolys(triangles);

    return polyData;
}
