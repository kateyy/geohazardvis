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

std::shared_ptr<PolyDataInput> Loader::loadFileAsPoints(const std::string & filename, t_UInt firstDataColumn)
{
    std::shared_ptr<PolyDataInput> input = std::make_shared<PolyDataInput>(filename);
    InputVector * parsedData = loadData(filename);
    input->setPolyData(*parsePoints(*parsedData, firstDataColumn));
    delete parsedData;

    return input;
}

std::shared_ptr<ProcessedInput> Loader::loadFileTriangulated(const std::string & filename, t_UInt firstDataColumn)
{
    std::shared_ptr<ProcessedInput> input = std::make_shared<ProcessedInput>(filename);
    InputVector * parsedData = loadData(filename);
    input->setPolyData(*parsePoints(*parsedData, firstDataColumn));
    delete parsedData;

    // create triangles from vertex list
    input->algorithm = vtkSmartPointer<vtkDelaunay2D>::New();
    input->algorithm->SetInputData(input->polyData());
    input->algorithm->Update();

    return input;
}

std::shared_ptr<PolyDataInput> Loader::loadIndexedTriangles(const std::string & inputFileName)
{
    vector<ReadData> readDataSets;
    string dataSetName;
    TextFileReader::read(inputFileName, dataSetName, readDataSets);

    // expect only vertex and index input data sets for now
    assert(readDataSets.size() == 2);
    
    std::shared_ptr<PolyDataInput> input = std::make_shared<PolyDataInput>(dataSetName);

    InputVector * indices = nullptr;
    InputVector * vertices = nullptr;

    for (ReadData & dataSet : readDataSets)
        if (dataSet.type == ContentType::vertices)
            vertices = &dataSet.data;
        else if (dataSet.type == ContentType::triangles)
            indices = &dataSet.data;

    assert(indices != nullptr && vertices != nullptr);

    input->setPolyData(*parseIndexedTriangles(*vertices, 0, 1, *indices, 0));

    return input;
}

std::shared_ptr<PolyDataInput> Loader::loadIndexedTriangles(
    const std::string & vertexFilename, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const std::string & indexFilename, t_UInt firstIndexColumn)
{
    std::shared_ptr<PolyDataInput> input = std::make_shared<PolyDataInput>(vertexFilename);
    InputVector * vertexData = loadData(vertexFilename);
    InputVector * indexData = loadData(indexFilename);

    input->setPolyData(*parseIndexedTriangles(
        *vertexData, vertexIndexColumn, firstVertexColumn,
        *indexData, firstIndexColumn));

    return input;
}

std::shared_ptr<GridDataInput> Loader::loadGrid(const std::string & inputFileName)
{
    string notused;
    vector<ReadData> readDataSets;
    TextFileReader::read(inputFileName, notused, readDataSets);

    assert(readDataSets.size() == 1);
    assert(readDataSets.begin()->type == ContentType::grid2d);
    InputVector * inputData = &readDataSets.begin()->data;
    std::shared_ptr<GridDataInput> input = dynamic_pointer_cast<GridDataInput>(readDataSets.begin()->input);
    assert(input);

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

std::shared_ptr<GridDataInput> Loader::loadGrid(const std::string & gridFilename, const std::string & xFilename, const std::string & yFilename)
{
    std::shared_ptr<GridDataInput> input = std::make_shared<GridDataInput>(gridFilename);
    InputVector * observation = loadData(gridFilename);
    InputVector * xDimensions = loadData(xFilename);
    InputVector * yDimensions = loadData(yFilename);

    assert(observation->size() > 0);
    assert(observation->at(0).size() > 0);

    int dimensions[3] = { static_cast<int>(observation->size()), static_cast<int>(observation->at(0).size()), 1 };

    input->bounds[0] = xDimensions->at(0).at(0);    // minX
    input->bounds[1] = xDimensions->at(xDimensions->size() - 1).at(0); // maxX
    input->bounds[2] = yDimensions->at(0).at(0);    // minY
    input->bounds[3] = yDimensions->at(0).at(yDimensions->at(0).size() - 1);    // maxY
    input->bounds[4] = 0;
    input->bounds[5] = 0;

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
            float value = observation->at(c).at(r);
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
