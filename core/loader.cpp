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
#include <vtkStructuredGridGeometryFilter.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkHeatmapItem.h>
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkImageData.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkActor.h>
#include <vtkPlaneSource.h>
#include <vtkExtractVOI.h>
#include <vtkInformationStringKey.h>

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

    int dimensions[3] = { static_cast<int>(observation->size()), static_cast<int>(observation->at(0).size()), 1 };

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

    vtkSmartPointer<vtkExtractVOI> voi = vtkSmartPointer<vtkExtractVOI>::New();
    voi->SetInputData(grid);
    voi->SetVOI(0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, 0);
    voi->Update();

    vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetLookupTable(lut);
    texture->SetInputData(voi->GetOutput());
    texture->MapColorScalarsThroughLookupTableOn();
    texture->InterpolateOn();
    texture->SetQualityTo32Bit();


    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetXResolution(dimensions[0]);
    plane->SetYResolution(dimensions[1]);
    float minXValue = xDimensions->at(0).at(0);
    float maxXValue = xDimensions->at(xDimensions->size() - 1).at(0);
    float minYValue = yDimensions->at(0).at(0);
    float maxYValue = yDimensions->at(0).at(yDimensions->at(0).size() - 1);
    plane->SetOrigin(0, 0, 0);
    plane->SetPoint1(-minXValue + maxXValue, 0, 0);
    plane->SetPoint2(0, -minYValue + maxYValue, 0);
    vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    planeMapper->SetInputConnection(plane->GetOutputPort());

    vtkSmartPointer<vtkActor> texturedPlane = vtkSmartPointer<vtkActor>::New();

    texturedPlane->SetMapper(planeMapper);
    texturedPlane->SetTexture(texture);

    input->prop = texturedPlane;

    return input;
}

std::shared_ptr<Context2DInput> Loader::loadGrid2DScene(const std::string & gridFilename, const std::string & xFilename, const std::string & yFilename)
{
    std::shared_ptr<Context2DInput> input = std::make_shared<Context2DInput>();
    ParsedData * observation = loadData(gridFilename);
    ParsedData * xDimensions = loadData(xFilename);
    ParsedData * yDimensions = loadData(yFilename);

    assert(observation->size() > 0);
    assert(observation->at(0).size() > 0);

    vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
    
    int dimensions[3] = { static_cast<int>(observation->size()), static_cast<int>(observation->at(0).size()), 1 };


    for (int c = 0; c < dimensions[0]; ++c) {
        vtkSmartPointer<vtkVariantArray> columnData = vtkSmartPointer<vtkVariantArray>::New();
        columnData->SetName(std::to_string(c).c_str());
        for (int r = 0; r < dimensions[1]; ++r) {
            columnData->InsertNextValue(observation->at(c).at(r));
        }
        table->AddColumn(columnData);
    }

    vtkSmartPointer<vtkHeatmapItem> heatmap = vtkSmartPointer<vtkHeatmapItem>::New();
    heatmap->SetCellHeight(3);
    heatmap->SetCellWidth(3);
    heatmap->SetTable(table);

    input->setContextItem(*heatmap);

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
