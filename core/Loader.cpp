#include "Loader.h"

#include <cmath>
#include <limits>

#include <QDebug>
#include <QList>
#include <QFileInfo>

#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkImageData.h>

#include "common/file_parser.h"
#include "TextFileReader.h"
#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/data_objects/AttributeVectorData.h>


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
    case ModelType::grid2D:
        return loadGrid2D(dataSetName, readDatasets);
    case ModelType::vectorGrid3D:
        return loadGrid3D(dataSetName, readDatasets);
    case ModelType::raw:
        return readRawFile(filename);
    default:
        cerr << "Warning: model type unsupported by the loader: " << int(inputInfo->type) << endl;
        return nullptr;
    }
}

DataObject * Loader::loadIndexedTriangles(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() >= 2);
    
    const InputVector * indices = nullptr;
    const InputVector * vertices = nullptr;
    const InputVector * centroid = nullptr;
    std::map<std::string, const InputVector *> vectorArrays;

    for (const ReadDataset & dataSet : datasets)
    {
        switch (dataSet.type)
        {
        case DatasetType::vertices: vertices = &dataSet.data;
            break;
        case DatasetType::indices: indices = &dataSet.data;
            break;
        case DatasetType::centroid: centroid = &dataSet.data;
            break;
        case DatasetType::vectors: vectorArrays.emplace(dataSet.attributeName, &dataSet.data);
            break;
        }
    }

    assert(indices != nullptr && vertices != nullptr);

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::Take(parseIndexedTriangles(*vertices, 0, 1, *indices, 0));

    if (centroid)
    {
        const vtkIdType numCells = polyData->GetNumberOfCells();
        assert(centroid->size() == 3);
        assert(size_t(numCells) == centroid->front().size());

        vtkSmartPointer<vtkFloatArray> a;
        a.TakeReference(parseFloatVector(*centroid, "centroid", 0, 2));

        polyData->GetCellData()->AddArray(a);
    }

    for (auto namedVector : vectorArrays)
    {
        const vtkIdType numCells = polyData->GetNumberOfCells();
        const auto vector = *namedVector.second;

        const vtkIdType tupleSize = vector.size();
        assert(size_t(numCells) == vector.front().size());

        vtkSmartPointer<vtkFloatArray> a;
        a.TakeReference(parseFloatVector(vector, namedVector.first.c_str(), 0, int(vector.size() - 1)));
        
        polyData->GetCellData()->AddArray(a);
    }

    return new PolyDataObject(name, polyData);
}

DataObject * Loader::loadGrid2D(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.begin()->type == DatasetType::grid2D);
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

DataObject * Loader::loadGrid3D(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.front().type == DatasetType::vectorGrid3D);
    const InputVector & data = datasets.front().data;
    // expecting point data and 3D vectors at minimum
    assert(data.size() >= 6);

    vtkSmartPointer<vtkPolyData> polyData;
    polyData.TakeReference(parsePoints(data, 0));

    vtkSmartPointer<vtkFloatArray> vectors;
    vectors.TakeReference(parseFloatVector(data, name.toLatin1().data(), 3, int(data.size() - 1)));
    
    polyData->GetPointData()->SetVectors(vectors);

    return new VectorGrid3DDataObject(name, polyData);
}

vtkPolyData * Loader::parsePoints(const InputVector & parsedData, t_UInt firstColumn)
{
    assert(parsedData.size() > firstColumn);

    VTK_CREATE(vtkPoints, points);

    size_t nbRows = parsedData.at(firstColumn).size();
    std::vector<vtkIdType> pointIds(nbRows);

    // copy triangle vertexes to VTK point list
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

DataObject * Loader::readRawFile(QString fileName)
{
    InputVector inputVectors;
    populateIOVectors(fileName.toStdString(), inputVectors);

    int numColumns = (int)inputVectors.size();
    if (numColumns == 0)
        return nullptr;
    int numRows = (int)inputVectors.at(0).size();
    if (numRows == 0)
        return nullptr;

    bool switchRowsColumns = numColumns > numRows;
    if (switchRowsColumns)
        std::swap(numColumns, numRows);

    vtkSmartPointer<vtkFloatArray> dataArray = vtkSmartPointer<vtkFloatArray>::New();
    dataArray->SetNumberOfComponents(numColumns);
    dataArray->SetNumberOfTuples(numRows);

    if (!switchRowsColumns)
        for (vtkIdType component = 0; component < numColumns; ++component)
            for (vtkIdType cellId = 0; cellId < numRows; ++cellId)
                dataArray->SetValue(cellId * numColumns + component, inputVectors.at(component).at(cellId));
    else
        for (vtkIdType component = 0; component < numColumns; ++component)
            for (vtkIdType cellId = 0; cellId < numRows; ++cellId)
                dataArray->SetValue(cellId * numColumns + component, inputVectors.at(cellId).at(component));

    QFileInfo fInfo(fileName);

    return new AttributeVectorData(fInfo.baseName(), dataArray);
}

vtkFloatArray * Loader::parseFloatVector(const InputVector & parsedData, QString arrayName, int firstColumn, int lastColumn)
{
    assert(firstColumn <= lastColumn);
    assert(parsedData.size() > lastColumn);
    vtkIdType numComponents = lastColumn - firstColumn + 1;
    vtkIdType numTuples = parsedData.at(lastColumn).size();

    for (auto ax : parsedData)
    {
        assert(ax.size() == size_t(numTuples));
    }

    vtkFloatArray * a = vtkFloatArray::New();
    a->SetName(arrayName.toLatin1().data());
    a->SetNumberOfComponents(numComponents);
    a->SetNumberOfTuples(numTuples);

    for (vtkIdType component = 0; component < numComponents; ++component)
        for (vtkIdType cellId = 0; cellId < numTuples; ++cellId)
            a->SetValue(cellId * numComponents + component, parsedData.at(component).at(cellId));

    return a;
}
