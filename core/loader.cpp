#include "loader.h"

#include <cmath>

#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkProperty.h>
#include <vtkCellArray.h>
#include <vtkStructuredGrid.h>
#include <vtkBoundingBox.h>

#include "input.h"
#include "common/file_parser.h"

std::shared_ptr<PolyDataInput> Loader::loadFileAsPoints(const std::string & filename, t_UInt firstDataColumn)
{
    std::shared_ptr<PolyDataInput> input = std::make_shared<PolyDataInput>(filename);
    ParsedData * parsedData = loadData(filename);
    input->setPolyData(*parsePoints(*parsedData, firstDataColumn));
    delete parsedData;

    return input;
}

std::shared_ptr<ProcessedInput> Loader::loadFileTriangulated(const std::string & filename, t_UInt firstDataColumn)
{
    std::shared_ptr<ProcessedInput> input = std::make_shared<ProcessedInput>(filename);
    ParsedData * parsedData = loadData(filename);
    input->setPolyData(*parsePoints(*parsedData, firstDataColumn));
    delete parsedData;

    // create triangles from vertex list
    input->algorithm = vtkSmartPointer<vtkDelaunay2D>::New();
    input->algorithm->SetInputData(input->polyData());
    input->algorithm->Update();

    return input;
}

std::shared_ptr<PolyDataInput> Loader::loadIndexedTriangles(
    const std::string & vertexFilename, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const std::string & indexFilename, t_UInt firstIndexColumn)
{
    std::shared_ptr<PolyDataInput> input = std::make_shared<PolyDataInput>(vertexFilename);
    ParsedData * vertexData = loadData(vertexFilename);
    ParsedData * indexData = loadData(indexFilename);

    input->setPolyData(*parseIndexedTriangles(
        *vertexData, vertexIndexColumn, firstVertexColumn,
        *indexData, firstIndexColumn));

    return input;
}

std::shared_ptr<DataSetInput> Loader::loadGrid(const std::string & gridFilename, const std::string & xFilename, const std::string & yFilename)
{
    std::shared_ptr<DataSetInput> input = std::make_shared<DataSetInput>(gridFilename);
    ParsedData * observation = loadData(gridFilename);
    ParsedData * xDimensions = loadData(xFilename);
    ParsedData * yDimensions = loadData(yFilename);

    assert(observation->size() > 0);
    assert(observation->at(0).size() > 0);

    int dimensions[3] = {observation->size(), static_cast<int>(observation->at(0).size()), 1};

    vtkStructuredGrid * sgrid = vtkStructuredGrid::New();
    sgrid->SetDimensions(dimensions);

    //vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    for (int c = 0; c < dimensions[1]; ++c) {
        vtkIdType cOffset = c * dimensions[0];
        for (int r = 0; r < dimensions[0]; ++r) {
            vtkIdType offset = r + cOffset;
            points->InsertPoint(offset, xDimensions->at(c).at(r), yDimensions->at(c).at(r), observation->at(c).at(r));
        }
    }

    sgrid->SetPoints(points);

    input->setDataSet(*sgrid);

    return input;
}

Loader::ParsedData * Loader::loadData(const std::string & filename)
{
    Loader::ParsedData * parsedData = new ParsedData();

    // load input file
    populateIOVectors(filename, *parsedData);

    assert(parsedData->size() > 0);
    assert(parsedData->at(0).size() > 0);

    return parsedData;
}

vtkPolyData * Loader::parsePoints(const ParsedData & parsedData, t_UInt firstColumn)
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

    vtkPolyData * polyData = vtkPolyData::New();
    polyData->SetPoints(points);
    polyData->SetVerts(vertices);

    return polyData;
}

vtkPolyData * Loader::parseIndexedTriangles(
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

    vtkPolyData * polyData = vtkPolyData::New();
    polyData->SetPoints(points);
    polyData->SetPolys(triangles);

    return polyData;
}
