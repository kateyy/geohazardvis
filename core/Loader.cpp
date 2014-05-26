#include "Loader.h"

#include <cmath>
#include <limits>

#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkCellArray.h>
#include <vtkPolyDataMapper.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>
#include <vtkPlaneSource.h>

#include "Input.h"
#include "common/file_parser.h"
#include "vtkhelper.h"
#include "TextFileReader.h"


using namespace std;

shared_ptr<Input> Loader::readFile(const string & filename)
{
    vector<ReadDataset> readDatasets;
    shared_ptr<Input> genericInput = TextFileReader::read(filename, readDatasets);
    if (!genericInput) {
        cerr << "Could not read the input file \"" << filename << "\"" << endl;
        return nullptr;
    }

    switch (genericInput->type) {
    case ModelType::triangles:
        loadIndexedTriangles(*static_cast<PolyDataInput*>(genericInput.get()), readDatasets);
        break;
    case ModelType::grid2d:
        loadGrid(*static_cast<GridDataInput*>(genericInput.get()), readDatasets);
        break;
    default:
        cerr << "Warning: model type unsupported by the loader: " << int(genericInput->type) << endl;
        return nullptr;
    }
    return genericInput;
}

void Loader::loadIndexedTriangles(PolyDataInput & input, const vector<ReadDataset> & datasets)
{
    // expect only vertex and index input data sets for now
    assert(datasets.size() == 2);
    
    const InputVector * indices = nullptr;
    const InputVector * vertices = nullptr;

    for (const ReadDataset & dataSet : datasets)
        if (dataSet.type == DatasetType::vertices)
            vertices = &dataSet.data;
        else if (dataSet.type == DatasetType::indices)
            indices = &dataSet.data;

    assert(indices != nullptr && vertices != nullptr);

    vtkSmartPointer<vtkPolyData> data = parseIndexedTriangles(*vertices, 0, 1, *indices, 0);

    input.setPolyData(data);
}

void Loader::loadGrid(GridDataInput & input, const vector<ReadDataset> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.begin()->type == DatasetType::grid2d);
    const InputVector * inputData = &datasets.begin()->data;

    int dimensions[3] = {static_cast<int>(inputData->size()), static_cast<int>(inputData->at(0).size()), 1};

    VTK_CREATE(vtkImageData, grid);
    grid->SetExtent(0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, 0);

    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();

    VTK_CREATE(vtkFloatArray, dataArray);
    dataArray->SetNumberOfComponents(1);
    dataArray->SetNumberOfTuples(grid->GetNumberOfPoints());
    for (int r = 0; r < dimensions[1]; ++r) {
        vtkIdType rOffset = r * dimensions[0];
        for (int c = 0; c < dimensions[0]; ++c) {
            vtkIdType id = c + rOffset;
            float value = inputData->at(c).at(r);
            if (value < minValue)
                minValue = value;
            if (value > maxValue)
                maxValue = value;
            dataArray->SetValue(id, value);
        }
    }

    input.setMinMaxValue(minValue, maxValue);

    grid->GetPointData()->SetScalars(dataArray);

    input.setData(grid);

    VTK_CREATE(vtkLookupTable, lut);
    lut->SetTableRange(minValue, maxValue);
    lut->SetNumberOfColors(static_cast<vtkIdType>(std::ceil(maxValue - minValue)) * 10);
    lut->SetHueRange(0.66667, 0.0);
    lut->SetValueRange(0.9, 0.9);
    lut->SetSaturationRange(1.0, 1.0);
    lut->SetAlphaRange(1.0, 1.0);
    lut->Build();

    input.lookupTable = lut;

    VTK_CREATE(vtkTexture, texture);
    texture->SetLookupTable(lut);
    texture->SetInputData(grid);
    texture->MapColorScalarsThroughLookupTableOn();
    texture->InterpolateOn();
    texture->SetQualityTo32Bit();

    double xExtend = input.bounds[1] - input.bounds[0];
    double yExtend = input.bounds[3] - input.bounds[2];

    VTK_CREATE(vtkPlaneSource, plane);
    plane->SetXResolution(dimensions[0]);
    plane->SetYResolution(dimensions[1]);
    plane->SetOrigin(input.bounds[0], input.bounds[2], 0);
    plane->SetPoint1(input.bounds[0] + xExtend, input.bounds[2], 0);
    plane->SetPoint2(input.bounds[0], input.bounds[2] + yExtend, 0);

    VTK_CREATE(vtkPolyDataMapper, planeMapper);
    planeMapper->SetInputConnection(plane->GetOutputPort());

    input.setMapper(planeMapper);
    input.setTexture(texture);
}

vtkSmartPointer<vtkPolyData> Loader::parsePoints(const InputVector & parsedData, t_UInt firstColumn)
{
    assert(parsedData.size() > firstColumn);

    VTK_CREATE(vtkPoints, points);

    size_t nbRows = parsedData.at(firstColumn).size();
    std::vector<vtkIdType> pointIds(nbRows);

    // copy triangle vertexes to vtk point list
    for (size_t row = 0; row < nbRows; ++row) {
        pointIds.at(row) = points->InsertNextPoint(parsedData[firstColumn][row], parsedData[firstColumn + 1][row], parsedData[firstColumn + 2][row]);
    }

    // Create the topology of the point (a vertex)
    VTK_CREATE(vtkCellArray, vertices);
    vertices->InsertNextCell(nbRows, pointIds.data());

    VTK_CREATE(vtkPolyData, resultPolyData);
    resultPolyData->SetPoints(points);
    resultPolyData->SetVerts(vertices);

    return resultPolyData;
}

vtkSmartPointer<vtkPolyData> Loader::parseIndexedTriangles(
    const InputVector & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const InputVector & parsedIndexData, t_UInt firstIndexColumn)
{
    VTK_CREATE(vtkPoints, points);

    size_t nbVertices = parsedVertexData[vertexIndexColumn].size();
    size_t nbTriangles = parsedIndexData[firstIndexColumn].size();

    std::vector<vtkIdType> pointIds(nbVertices);

    // to let the internal indexes start with 0
    t_UInt indexOffset = std::llround(parsedVertexData[vertexIndexColumn][0]);

    for (size_t row = 0; row < nbVertices; ++row) {
        points->InsertNextPoint(parsedVertexData[firstVertexColumn][row], parsedVertexData[firstVertexColumn + 1][row], parsedVertexData[firstVertexColumn + 2][row]);
        pointIds.at(row) = std::llround(parsedVertexData[vertexIndexColumn][row]) - indexOffset;
    }

    VTK_CREATE(vtkCellArray, triangles);
    VTK_CREATE(vtkTriangle, triangle);
    for (size_t row = 0; row < nbTriangles; ++row) {
        // set the indexes for the three triangle vertexes
        // hopefully, these indexes map to the vertex indexes from parsedVertexData
        triangle->GetPointIds()->SetId(0, std::llround(parsedIndexData[firstIndexColumn][row]) - indexOffset);
        triangle->GetPointIds()->SetId(1, std::llround(parsedIndexData[firstIndexColumn+1][row]) - indexOffset);
        triangle->GetPointIds()->SetId(2, std::llround(parsedIndexData[firstIndexColumn+2][row]) - indexOffset);
        triangles->InsertNextCell(triangle);    // this copies the triangle data into the list
    }

    VTK_CREATE(vtkPolyData, resultPolyData);
    resultPolyData->SetPoints(points);
    resultPolyData->SetPolys(triangles);

    return resultPolyData;
}
