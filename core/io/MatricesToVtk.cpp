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
#include <vtkDoubleArray.h>
#include <vtkCellArray.h>

#include <vtkTriangle.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/RawVectorData.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/io/TextFileReader.h>
#include <core/io/FileParser.h>


using namespace io;


namespace
{
template<typename T>
T positive_modulo(T i, T n)
{
    return (i % n + n) % n;
}

vtkSmartPointer<vtkDataArray> createDataArray(int vtk_dataType)
{
    return vtkSmartPointer<vtkDataArray>::Take(vtkDataArray::CreateDataArray(vtk_dataType));
}

}


std::unique_ptr<DataObject> MatricesToVtk::loadIndexedTriangles(const QString & name, const std::vector<ReadDataSet> & datasets)
{
    assert(datasets.size() >= 2);

    const InputVector * indices = nullptr;
    const InputVector * vertices = nullptr;
    std::map<QString, const InputVector *> vectorArrays;

    for (const ReadDataSet & dataSet : datasets)
    {
        switch (dataSet.type)
        {
        case DataSetType::vertices: vertices = &dataSet.data;
            break;
        case DataSetType::indices: indices = &dataSet.data;
            break;
        case DataSetType::vectors: vectorArrays.emplace(dataSet.attributeName, &dataSet.data);
            break;
        default:
            assert(false);
        }
    }

    assert(indices != nullptr && vertices != nullptr);

    vtkSmartPointer<vtkPolyData> polyData = parseIndexedTriangles(*vertices, 0, 1, *indices, 0);

    for (auto && namedVector : vectorArrays)
    {
        const auto vector = *namedVector.second;

        assert(size_t(polyData->GetNumberOfCells()) == vector.front().size());

        auto a = parseFloatVector(vector, namedVector.first, 0, int(vector.size() - 1));

        polyData->GetCellData()->AddArray(a);
    }

    return std::make_unique<PolyDataObject>(name, *polyData);
}

std::unique_ptr<DataObject> MatricesToVtk::loadGrid2D(const QString & name, const std::vector<ReadDataSet> & datasets)
{
    assert(datasets.size() == 1);
    const auto dataDef = datasets.front();
    assert(dataDef.type == DataSetType::grid2D);
    const InputVector & inputData = dataDef.data;
    auto metaDataImage = vtkImageData::SafeDownCast(dataDef.vtkMetaData);
    assert(metaDataImage);

    int dimensions[3];
    metaDataImage->GetDimensions(dimensions);
    if (dimensions[0] != static_cast<int>(inputData.size())
        || dimensions[1] != static_cast<int>(inputData.at(0).size())
        || dimensions[2] != 1)
    {
        qDebug() << "Image data specification and the size of the provided data set do not match.";
        return nullptr;
    }

    auto parsedImage = parseGrid2D(inputData);
    parsedImage->GetPointData()->GetScalars()->SetName(name.toUtf8());

    return std::make_unique<ImageDataObject>(name, *parsedImage);
}

std::unique_ptr<DataObject> MatricesToVtk::loadGrid3D(const QString & name, const std::vector<ReadDataSet> & datasets)
{
    assert(datasets.size() == 1);
    assert(datasets.front().type == DataSetType::vectorGrid3D);
    const InputVector & data = datasets.front().data;
    // expecting point data and 3D vectors at minimum
    assert(data.size() >= 6);
    const int numComponents = static_cast<int>(data.size() - 3);

    assert((data[0].size() == data[1].size()) && (data[0].size() == data[2].size()));

    if (data[0].size() > (size_t)std::numeric_limits<vtkIdType>::max())
    {
        qDebug() << "cannot read data set (too large to count with vtkIdType, " + QString::number(sizeof(vtkIdType)) + ")";
        return nullptr;
    }

    const vtkIdType numPoints = static_cast<vtkIdType>(data[0].size());

    // some assumptions on the file format:
    // it's a regular grid...

    double origin[3] = { data[0][0], data[1][0], data[2][0] };

    int axis[3] = { -1, -1, -1 };   // axes, ordered by majority

    if (data[0][1] != origin[0])
        axis[0] = 0;    // x first
    else if (data[1][1] != origin[1])
        axis[0] = 1;    // y first
    else
        axis[0] = 2;    // z first

    double spacing[3] = { -1, -1, -1 };
    int extent[3] = { -1, -1, -1 };

    int firstAxis = axis[0];

    spacing[firstAxis] = std::abs(origin[firstAxis] - data[firstAxis][1]);

    // count first axis and get next spacing
    extent[firstAxis] = 0;
    int remainingAxes[2] = { positive_modulo(firstAxis + 1, 3), positive_modulo(firstAxis + 2, 3) };
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        // check for next axis
        t_FP nextCoord = data[remainingAxes[0]][i];
        if (nextCoord != origin[remainingAxes[0]])
        {
            axis[1] = remainingAxes[0];
            axis[2] = remainingAxes[1];
            spacing[axis[1]] = std::abs(nextCoord - origin[axis[1]]);
            break;
        }

        nextCoord = data[remainingAxes[1]][i];
        if (nextCoord != origin[remainingAxes[1]])
        {
            axis[1] = remainingAxes[1];
            axis[2] = remainingAxes[0];
            spacing[axis[1]] = std::abs(nextCoord - origin[axis[1]]);
            break;
        }

        ++extent[firstAxis];
    }
    assert(spacing[axis[0]] > 0 && extent[firstAxis] > 0);


    // count second axis and get last spacing
    extent[axis[1]] = 0;
    for (vtkIdType i = 0; i < numPoints; i += extent[axis[0]])
    {
        t_FP nextCoord = data[axis[2]][i];
        if (nextCoord != origin[axis[2]])
        {
            spacing[axis[2]] = std::abs(origin[axis[2]] - nextCoord);
            break;
        }
        ++extent[axis[1]];
    }

    assert(extent[axis[1]] > 0);

    // get number of slices
    // we should be able to compute them without counting, if the grid is complete

    // didn't get last spacing -> only one slice
    if (spacing[axis[2]] < 0)
    {
        spacing[axis[2]] = 0;
        extent[axis[2]] = 1;
    }
    else
    {
        double f_lastExtent = double(numPoints) / double(extent[axis[0]] * extent[axis[1]]);
        double intPart;
        double f_floatPart = std::modf(f_lastExtent, &intPart);
        if (f_floatPart < std::numeric_limits<double>::epsilon())
            extent[axis[2]] = static_cast<int>(intPart);
    }

    if (extent[axis[2]] < 1 || (numPoints != extent[0] * extent[1] * extent[2]))
    {
        qDebug() << "cannot read incomplete grid data set";
        return nullptr;
    }

    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetOrigin(origin);
    image->SetExtent(0, extent[0] - 1, 0, extent[1] - 1, 0, extent[2] - 1);
    image->SetSpacing(spacing);

    auto vectorData = vtkSmartPointer<vtkFloatArray>::New();
    vectorData->SetNumberOfComponents(numComponents);
    vectorData->SetNumberOfTuples(numPoints);
    vectorData->SetName(name.toUtf8().data());

    image->GetPointData()->SetScalars(vectorData);

    if (numComponents == 3)
    {
        image->GetPointData()->SetVectors(vectorData);
    }

    // iterate over source data, calculate indexes for vtkImageData
    // data in vtkImageData stored in tupleValues - x - y - z - order

    vtkIdType sourceIndex = 0;
    int imageCoord[3];

    for (imageCoord[axis[2]] = 0; imageCoord[axis[2]] < extent[axis[2]]; ++imageCoord[axis[2]])
    {
        for (imageCoord[axis[1]] = 0; imageCoord[axis[1]] < extent[axis[1]]; ++imageCoord[axis[1]])
        {
            for (imageCoord[axis[0]] = 0; imageCoord[axis[0]] < extent[axis[0]]; ++imageCoord[axis[0]])
            {
                float * scalar = reinterpret_cast<float *>(image->GetScalarPointer(imageCoord));

                for (int c = 0; c < numComponents; ++c)
                    scalar[c] = static_cast<float>(data.at(3 + c).at(sourceIndex));

                ++sourceIndex;
            }
        }
    }

    return std::make_unique<VectorGrid3DDataObject>(name, *image);
}

vtkSmartPointer<vtkImageData> MatricesToVtk::parseGrid2D(const io::InputVector & inputData, int vtk_dataType)
{
    auto image = vtkSmartPointer<vtkImageData>::New();
    if (inputData.size() == 0 || inputData.at(0).size() == 0)
    {
        return image;
    }

    const std::array<int, 3> dimensions = {
        static_cast<int>(inputData.size()),
        static_cast<int>(inputData.at(0).size()),
        1
    };

    image->SetDimensions(dimensions.data());
    image->AllocateScalars(vtk_dataType, 1);
    for (int x = 0; x < dimensions[0]; ++x)
    {
        for (int y = 0; y < dimensions[1]; ++y)
        {
            image->SetScalarComponentFromDouble(x, y, 0, 0, inputData.at(x).at(y));
        }
    }

    return image;
}

vtkSmartPointer<vtkPolyData> MatricesToVtk::parsePoints(
    const InputVector & parsedData, size_t firstColumn,
    int vtk_dataType /*= VTK_FLOAT*/)
{
    assert(parsedData.size() > firstColumn);

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetDataType(vtk_dataType);

    size_t nbRows = parsedData.at(firstColumn).size();
    std::vector<vtkIdType> pointIds(nbRows);

    // copy triangle vertexes to VTK point list
    for (size_t row = 0; row < nbRows; ++row)
    {
        pointIds.at(row) = points->InsertNextPoint(parsedData[firstColumn][row], parsedData[firstColumn + 1][row], parsedData[firstColumn + 2][row]);
    }

    // Create the topology of the point (a vertex)
    auto vertices = vtkSmartPointer<vtkCellArray>::New();
    vertices->InsertNextCell(nbRows, pointIds.data());

    auto resultPolyData = vtkSmartPointer<vtkPolyData>::New();
    resultPolyData->SetPoints(points);
    resultPolyData->SetVerts(vertices);

    return resultPolyData;
}

vtkSmartPointer<vtkPolyData> MatricesToVtk::parseIndexedTriangles(
    const InputVector & parsedVertexData, size_t vertexIndexColumn, size_t firstVertexColumn,
    const InputVector & parsedIndexData, size_t firstIndexColumn,
    int vtk_dataType /*= VTK_FLOAT*/)
{
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetDataType(vtk_dataType);

    size_t nbVertices = parsedVertexData[vertexIndexColumn].size();
    size_t nbTriangles = parsedIndexData[firstIndexColumn].size();

    // to let the internal indexes start with 0
    size_t indexOffset = std::llround(parsedVertexData[vertexIndexColumn][0]);

    for (size_t row = 0; row < nbVertices; ++row)
    {
        points->InsertNextPoint(parsedVertexData[firstVertexColumn][row], parsedVertexData[firstVertexColumn + 1][row], parsedVertexData[firstVertexColumn + 2][row]);
    }

    auto triangles = vtkSmartPointer<vtkCellArray>::New();
    auto triangle = vtkSmartPointer<vtkTriangle>::New();
    for (size_t row = 0; row < nbTriangles; ++row)
    {
        // set the indexes for the three triangle vertexes
        // hopefully, these indexes map to the vertex indexes from parsedVertexData
        triangle->GetPointIds()->SetId(0, std::llround(parsedIndexData[firstIndexColumn][row]) - indexOffset);
        triangle->GetPointIds()->SetId(1, std::llround(parsedIndexData[firstIndexColumn + 1][row]) - indexOffset);
        triangle->GetPointIds()->SetId(2, std::llround(parsedIndexData[firstIndexColumn + 2][row]) - indexOffset);
        triangles->InsertNextCell(triangle);    // this copies the triangle data into the list
    }

    auto resultPolyData = vtkSmartPointer<vtkPolyData>::New();
    resultPolyData->SetPoints(points);
    resultPolyData->SetPolys(triangles);

    return resultPolyData;
}

std::unique_ptr<DataObject> MatricesToVtk::readRawFile(const QString & fileName)
{
    InputVector inputVectors;
    FileParser::populateIOVectors(fileName.toStdString(), inputVectors);

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
                dataArray->SetValue(cellId * numColumns + component,
                    static_cast<float>(inputVectors.at(component).at(cellId)));
    else
        for (vtkIdType component = 0; component < numColumns; ++component)
            for (vtkIdType cellId = 0; cellId < numRows; ++cellId)
                dataArray->SetValue(cellId * numColumns + component,
                    static_cast<float>(inputVectors.at(cellId).at(component)));

    QFileInfo fInfo(fileName);

    return std::make_unique<RawVectorData>(fInfo.baseName(), *dataArray);
}

vtkSmartPointer<vtkDataArray> MatricesToVtk::parseFloatVector(
    const InputVector & parsedData, const QString & arrayName, size_t firstColumn, size_t lastColumn,
    int vtk_dataType /*= VTK_FLOAT*/)
{
    assert(firstColumn <= lastColumn);
    assert(parsedData.size() > lastColumn);
    int numComponents = static_cast<int>(lastColumn - firstColumn + 1);
    vtkIdType numTuples = parsedData.at(lastColumn).size();

    for (auto ax : parsedData)
    {
        assert(ax.size() == size_t(numTuples));
    }

    auto a = createDataArray(vtk_dataType);
    a->SetName(arrayName.toUtf8().data());
    a->SetNumberOfComponents(numComponents);
    a->SetNumberOfTuples(numTuples);

    std::vector<io::t_FP> tuple(numComponents);

    for (vtkIdType cellId = 0; cellId < numTuples; ++cellId)
    {
        for (vtkIdType component = 0; component < numComponents; ++component)
        {
            tuple[component] = parsedData.at(component).at(cellId);
        }
        a->SetTuple(cellId, tuple.data());
    }

    return a;
}
