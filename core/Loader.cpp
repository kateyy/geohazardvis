#include "Loader.h"

#include <cmath>
#include <limits>

#include <QDebug>

#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkImageData.h>

#include "common/file_parser.h"
#include "vtkhelper.h"
#include "TextFileReader.h"
#include <data_objects/PolyDataObject.h>
#include <data_objects/ImageDataObject.h>


DataObject * Loader::readFile(QString filename)
{
    std::vector<ReadDataset> readDatasets;
    std::shared_ptr<InputFileInfo> inputInfo = TextFileReader::read(filename.toStdString(), readDatasets);
    if (!inputInfo)
    {
        qDebug() << "Could not read the input file \"" << filename << "\"";
        return nullptr;
    }

    QString dataSetName = QString::fromStdString(inputInfo->name);
    switch (inputInfo->type)
    {
    case ModelType::triangles:
        return loadIndexedTriangles(dataSetName, readDatasets);
    case ModelType::grid2d:
        return loadGrid(dataSetName, readDatasets);
    default:
        cerr << "Warning: model type unsupported by the loader: " << int(inputInfo->type) << endl;
        return nullptr;
    }
}

PolyDataObject * Loader::loadIndexedTriangles(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() >= 2);
    
    const InputVector * indices = nullptr;
    const InputVector * vertices = nullptr;
    const InputVector * centroid = nullptr;

    for (const ReadDataset & dataSet : datasets)
    {
        switch (dataSet.type)
        {
        case DatasetType::vertices: vertices = &dataSet.data;
            break;
        case DatasetType::indices: indices = &dataSet.data;
            break;
        case DatasetType::centroid: centroid = &dataSet.data;
        }
    }

    assert(indices != nullptr && vertices != nullptr);

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::Take(parseIndexedTriangles(*vertices, 0, 1, *indices, 0));

    if (centroid)
    {
        const vtkIdType numCells = polyData->GetNumberOfCells();
        assert(centroid->size() == 3);
        assert(size_t(numCells) == centroid->front().size());

        vtkSmartPointer<vtkFloatArray> a = vtkSmartPointer<vtkFloatArray>::New();
        a->SetName("centroid");
        a->SetNumberOfComponents(3);
        a->SetNumberOfTuples(numCells);

        const std::vector<t_FP> & xs = centroid->at(0);
        const std::vector<t_FP> & ys = centroid->at(1);
        const std::vector<t_FP> & zs = centroid->at(2);

        for (vtkIdType i = 0; i < numCells; ++i)
            a->SetTuple3(i, xs.at(i), ys.at(i), zs.at(i));

        polyData->GetCellData()->AddArray(a);
    }

    return new PolyDataObject(name, polyData);
}

ImageDataObject * Loader::loadGrid(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.begin()->type == DatasetType::grid2d);
    const InputVector * inputData = &datasets.begin()->data;

    int dimensions[3] = { static_cast<int>(inputData->size()), static_cast<int>(inputData->at(0).size()), 1 };

    VTK_CREATE(vtkImageData, grid);
    grid->SetExtent(0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, 0);

    VTK_CREATE(vtkFloatArray, dataArray);
    dataArray->SetNumberOfComponents(1);
    dataArray->SetNumberOfTuples(grid->GetNumberOfPoints());
    for (int r = 0; r < dimensions[1]; ++r)
    {
        vtkIdType rOffset = r * dimensions[0];
        for (int c = 0; c < dimensions[0]; ++c)
        {
            vtkIdType id = c + rOffset;
            float value = inputData->at(c).at(r);
            dataArray->SetValue(id, value);
        }
    }

    grid->GetPointData()->SetScalars(dataArray);

    return new ImageDataObject(name, grid);
}

vtkPolyData * Loader::parsePoints(const InputVector & parsedData, t_UInt firstColumn)
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

    vtkPolyData * resultPolyData = vtkPolyData::New();
    resultPolyData->SetPoints(points);
    resultPolyData->SetVerts(vertices);

    return resultPolyData;
}

vtkPolyData * Loader::parseIndexedTriangles(
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

    vtkPolyData * resultPolyData = vtkPolyData::New();
    resultPolyData->SetPoints(points);
    resultPolyData->SetPolys(triangles);

    return resultPolyData;
}
