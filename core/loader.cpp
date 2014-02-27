#include "loader.h"

#include <cmath>

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

#include "input.h"
#include "common/file_parser.h"
#include "textfilereader.h"

using namespace std;

shared_ptr<PolyDataInput> Loader::loadIndexedTriangles(const std::string & inputFileName)
{
    vector<ReadDataset> readDataSets;
    string dataSetName;
    shared_ptr<Input> genInput = TextFileReader::read(inputFileName, readDataSets);
    shared_ptr<PolyDataInput> input = dynamic_pointer_cast<PolyDataInput>(genInput);
    assert(input);

    // expect only vertex and index input data sets for now
    assert(readDataSets.size() == 2);
    

    InputVector * indices = nullptr;
    InputVector * vertices = nullptr;

    for (ReadDataset & dataSet : readDataSets)
        if (dataSet.type == DatasetType::vertices)
            vertices = &dataSet.data;
        else if (dataSet.type == DatasetType::indices)
            indices = &dataSet.data;

    assert(indices != nullptr && vertices != nullptr);

    input->setPolyData(*parseIndexedTriangles(*vertices, 0, 1, *indices, 0));

    return input;
}

std::shared_ptr<GridDataInput> Loader::loadGrid(const std::string & inputFileName)
{
    vector<ReadDataset> readDataSets;
    std::shared_ptr<Input> genInput = TextFileReader::read(inputFileName, readDataSets);
    std::shared_ptr<GridDataInput> input = dynamic_pointer_cast<GridDataInput>(genInput);
    assert(input);

    assert(readDataSets.size() == 1);
    assert(readDataSets.begin()->type == DatasetType::grid2d);
    InputVector * inputData = &readDataSets.begin()->data;

    int dimensions[3] = {static_cast<int>(inputData->size()), static_cast<int>(inputData->at(0).size()), 1};

    vtkSmartPointer<vtkImageData> grid = vtkSmartPointer<vtkImageData>::New();
    grid->SetExtent(0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, 0);

    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();

    vtkSmartPointer<vtkFloatArray> dataArray = vtkSmartPointer<vtkFloatArray>::New();
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

    input->setMinMaxValue(minValue, maxValue);

    grid->GetPointData()->SetScalars(dataArray);

    input->setData(*grid);

    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(minValue, maxValue);
    lut->SetNumberOfColors(static_cast<vtkIdType>(std::ceil(maxValue - minValue)) * 10);
    lut->SetHueRange(0.66667, 0.0);
    lut->SetValueRange(0.9, 0.9);
    lut->SetSaturationRange(1.0, 1.0);
    lut->SetAlphaRange(1.0, 1.0);
    lut->Build();

    input->lookupTable = lut;

    vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetLookupTable(lut);
    texture->SetInputData(grid);
    texture->MapColorScalarsThroughLookupTableOn();
    texture->InterpolateOn();
    texture->SetQualityTo32Bit();

    double xExtend = input->bounds[1] - input->bounds[0];
    double yExtend = input->bounds[3] - input->bounds[2];

    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetXResolution(dimensions[0]);
    plane->SetYResolution(dimensions[1]);
    plane->SetOrigin(input->bounds[0], input->bounds[2], 0);
    plane->SetPoint1(input->bounds[0] + xExtend, input->bounds[2], 0);
    plane->SetPoint2(input->bounds[0], input->bounds[2] + yExtend, 0);

    vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    planeMapper->SetInputConnection(plane->GetOutputPort());

    input->setMapper(*planeMapper);
    input->setTexture(*texture);

    return input;
}

Loader::InputVector * Loader::loadData(const std::string & filename)
{
    InputVector * parsedData = new InputVector();

    // load input file
    populateIOVectors(filename, *parsedData);

    assert(parsedData->size() > 0);
    assert(parsedData->at(0).size() > 0);

    return parsedData;
}

vtkPolyData * Loader::parsePoints(const InputVector & parsedData, t_UInt firstColumn)
{
    assert(parsedData.size() > firstColumn);

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    size_t nbRows = parsedData.at(firstColumn).size();
    std::vector<vtkIdType> pointIds(nbRows);

    // copy triangle vertexes to vtk point list
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
    const InputVector & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const InputVector & parsedIndexData, t_UInt firstIndexColumn)
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    size_t nbVertices = parsedVertexData[vertexIndexColumn].size();
    size_t nbTriangles = parsedIndexData[firstIndexColumn].size();

    std::vector<vtkIdType> pointIds(nbVertices);

    // to let the internal indexes start with 0
    t_UInt indexOffset = std::llround(parsedVertexData[vertexIndexColumn][0]);

    for (size_t row = 0; row < nbVertices; ++row) {
        points->InsertNextPoint(parsedVertexData[firstVertexColumn][row], parsedVertexData[firstVertexColumn + 1][row], parsedVertexData[firstVertexColumn + 2][row]);
        pointIds.at(row) = std::llround(parsedVertexData[vertexIndexColumn][row]) - indexOffset;
    }

    vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
    for (size_t row = 0; row < nbTriangles; ++row) {
        // set the indexes for the three triangle vertexes
        // hopefully, these indexes map to the vertex indexes from parsedVertexData
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
