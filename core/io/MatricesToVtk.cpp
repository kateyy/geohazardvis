#include "MatricesToVtk.h"

#include <cassert>
#include <map>

#include <QDebug>
#include <QFileInfo>

#include <vtkSmartPointer.h>

#include <vtkPolyData.h>
#include <vtkImageData.h>

#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkCellArray.h>

#include <vtkTriangle.h>

#include <core/vtkhelper.h>
#include <core/common/file_parser.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/RawVectorData.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/io/TextFileReader.h>


DataObject * MatricesToVtk::loadIndexedTriangles(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() >= 2);

    const InputVector * indices = nullptr;
    const InputVector * vertices = nullptr;
    std::map<std::string, const InputVector *> vectorArrays;

    for (const ReadDataset & dataSet : datasets)
    {
        switch (dataSet.type)
        {
        case DatasetType::vertices: vertices = &dataSet.data;
            break;
        case DatasetType::indices: indices = &dataSet.data;
            break;
        case DatasetType::vectors: vectorArrays.emplace(dataSet.attributeName, &dataSet.data);
            break;
        default:
            assert(false);
        }
    }

    assert(indices != nullptr && vertices != nullptr);

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::Take(parseIndexedTriangles(*vertices, 0, 1, *indices, 0));

    for (auto namedVector : vectorArrays)
    {
        const auto vector = *namedVector.second;

        assert(size_t(polyData->GetNumberOfCells()) == vector.front().size());

        vtkSmartPointer<vtkFloatArray> a;
        a.TakeReference(parseFloatVector(vector, namedVector.first.c_str(), 0, int(vector.size() - 1)));

        polyData->GetCellData()->AddArray(a);
    }

    return new PolyDataObject(name, polyData);
}

DataObject * MatricesToVtk::loadGrid2D(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.begin()->type == DatasetType::grid2D);
    const InputVector * inputData = &datasets.begin()->data;

    int dimensions[3] = { static_cast<int>(inputData->size()), static_cast<int>(inputData->at(0).size()), 1 };

    VTK_CREATE(vtkImageData, grid);
    // assign scalars to points
    grid->SetExtent(0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, 0);

    VTK_CREATE(vtkFloatArray, cellArray);
    cellArray->SetName(name.toLatin1().data());
    cellArray->SetNumberOfComponents(1);
    cellArray->SetNumberOfTuples(dimensions[0] * dimensions[1] * dimensions[2]);
    for (int r = 0; r < dimensions[1]; ++r)
    {
        vtkIdType rOffset = r * dimensions[0];
        for (int c = 0; c < dimensions[0]; ++c)
        {
            vtkIdType id = c + rOffset;
            float value = inputData->at(c).at(r);
            cellArray->SetValue(id, value);
        }
    }

    grid->GetPointData()->SetScalars(cellArray);

    return new ImageDataObject(name, grid);
}

DataObject * MatricesToVtk::loadGrid3D(QString name, const std::vector<ReadDataset> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.front().type == DatasetType::vectorGrid3D);
    const InputVector & data = datasets.front().data;
    // expecting point data and 3D vectors at minimum
    assert(data.size() >= 6);
    const int numComponents = static_cast<int>(data.size() - 3);

    const std::vector<t_FP> & coordX = data[0];
    const std::vector<t_FP> & coordY = data[1];
    const std::vector<t_FP> & coordZ = data[2];
    assert((coordX.size() == coordY.size()) && (coordX.size() == coordZ.size()));

    if (coordX.size() > (size_t)std::numeric_limits<vtkIdType>::max())
    {
        qDebug() << "cannot read data set (too large to count with vtkIdType, " + QString::number(sizeof(vtkIdType)) + ")";
        return nullptr;
    }

    const vtkIdType numPoints = static_cast<vtkIdType>(coordX.size());

    // some assumptions on the file format:
    // starts with continues values along the y axis
    // creating columns along the x axis
    // creating slices along the z axis
    //
    // it's an regular grid...

    double originX = coordX[0];
    double originY = coordY[0];
    double originZ = coordZ[0];

    // get column spacing
    double xSpacing = -1, ySpacing = -1, zSpacing = -1;
    vtkIdType xExtent = -1, yExtent = -1, zExtent = -1;
    ySpacing = std::abs(originX - coordY[1]);

    // count columns and get row spacing
    yExtent = 0;
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        t_FP nextX = coordX.at(i);
        if (nextX != originX)
        {
            xSpacing = std::abs(originX - nextX);
            break;
        }
        ++yExtent;
    }
    assert(xSpacing > 0 && yExtent > 0);

    // count rows and get slice spacing
    xExtent = 0;
    for (vtkIdType i = 0; i < numPoints; i += yExtent)
    {
        t_FP nextZ = coordZ.at(i);
        if (nextZ != originZ)
        {
            zSpacing = std::abs(originZ - nextZ);
            break;
        }
        ++xExtent;
    }

    assert(xExtent > 0);

    // get number of z-slices
    // we should be able to compute them without counting, if the grid is complete

    // didn't get zSpacing -> only one slice
    if (zSpacing < 0)
    {
        zSpacing = 0;
        zExtent = 1;
    }
    else
    {
        double f_zExtent = double(numPoints) / (xExtent * yExtent);
        double intPart;
        double f_floatPart = std::modf(f_zExtent, &intPart);
        if (f_floatPart < std::numeric_limits<double>::epsilon())
            zExtent = static_cast<vtkIdType>(intPart);
    }

    if (zExtent < 1 || (numPoints != xExtent * yExtent * zExtent))
    {
        qDebug() << "cannot read incomplete grid data set";
        return nullptr;
    }

    VTK_CREATE(vtkFloatArray, vectorData);
    vectorData->SetNumberOfComponents(numComponents);
    vectorData->SetNumberOfTuples(numPoints);
    vectorData->SetName(name.toLatin1().data());

    float * tuple = new float[numComponents];

    vtkIdType numSliceValues = xExtent * yExtent;
    for (vtkIdType z = 0; z < zExtent; ++z)
    {
        vtkIdType sliceOffset = z * numSliceValues;
        for (vtkIdType x = 0; x < xExtent; ++x)
        {
            vtkIdType columnOffset = x * yExtent;
            for (vtkIdType y = 0; y < yExtent; ++y)
            {
                vtkIdType index = sliceOffset + columnOffset + y;
                for (int i = 0; i < numComponents; ++i)
                    tuple[i] = data.at(3 + i).at(index);
                vectorData->SetTuple(index, tuple);
            }
        }
    }

    delete[] tuple;

    VTK_CREATE(vtkImageData, image);
    image->SetOrigin(originX, originY, originZ);
    image->SetExtent(0, xExtent - 1, 0, yExtent - 1, 0, zExtent - 1);
    image->SetSpacing(xSpacing, ySpacing, zSpacing);

    image->GetPointData()->SetVectors(vectorData);

    return new VectorGrid3DDataObject(name, image);
}

vtkPolyData * MatricesToVtk::parsePoints(const InputVector & parsedData, t_UInt firstColumn)
{
    assert(parsedData.size() > firstColumn);

    VTK_CREATE(vtkPoints, points);

    size_t nbRows = parsedData.at(firstColumn).size();
    std::vector<vtkIdType> pointIds(nbRows);

    // copy triangle vertexes to VTK point list
    for (size_t row = 0; row < nbRows; ++row)
    {
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

vtkPolyData * MatricesToVtk::parseIndexedTriangles(
    const InputVector & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
    const InputVector & parsedIndexData, t_UInt firstIndexColumn)
{
    VTK_CREATE(vtkPoints, points);

    size_t nbVertices = parsedVertexData[vertexIndexColumn].size();
    size_t nbTriangles = parsedIndexData[firstIndexColumn].size();

    std::vector<vtkIdType> pointIds(nbVertices);

    // to let the internal indexes start with 0
    t_UInt indexOffset = std::llround(parsedVertexData[vertexIndexColumn][0]);

    for (size_t row = 0; row < nbVertices; ++row)
    {
        points->InsertNextPoint(parsedVertexData[firstVertexColumn][row], parsedVertexData[firstVertexColumn + 1][row], parsedVertexData[firstVertexColumn + 2][row]);
        pointIds.at(row) = std::llround(parsedVertexData[vertexIndexColumn][row]) - indexOffset;
    }

    VTK_CREATE(vtkCellArray, triangles);
    VTK_CREATE(vtkTriangle, triangle);
    for (size_t row = 0; row < nbTriangles; ++row)
    {
        // set the indexes for the three triangle vertexes
        // hopefully, these indexes map to the vertex indexes from parsedVertexData
        triangle->GetPointIds()->SetId(0, std::llround(parsedIndexData[firstIndexColumn][row]) - indexOffset);
        triangle->GetPointIds()->SetId(1, std::llround(parsedIndexData[firstIndexColumn + 1][row]) - indexOffset);
        triangle->GetPointIds()->SetId(2, std::llround(parsedIndexData[firstIndexColumn + 2][row]) - indexOffset);
        triangles->InsertNextCell(triangle);    // this copies the triangle data into the list
    }

    vtkPolyData * resultPolyData = vtkPolyData::New();
    resultPolyData->SetPoints(points);
    resultPolyData->SetPolys(triangles);

    return resultPolyData;
}

DataObject * MatricesToVtk::readRawFile(QString fileName)
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

    return new RawVectorData(fInfo.baseName(), dataArray);
}

vtkFloatArray * MatricesToVtk::parseFloatVector(const InputVector & parsedData, QString arrayName, int firstColumn, int lastColumn)
{
    assert(firstColumn <= lastColumn);
    assert(parsedData.size() > unsigned(lastColumn));
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
