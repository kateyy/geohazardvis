#pragma once

#include <memory>

#include <core/core_api.h>
#include <core/io/types.h>


class QString;
class vtkPolyData;
class vtkFloatArray;
template<typename T> class vtkSmartPointer;
class DataObject;


class CORE_API MatricesToVtk
{
public:
    static std::unique_ptr<DataObject> loadIndexedTriangles(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadDEM(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadGrid2D(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadGrid3D(const QString & name, const std::vector<io::ReadDataSet> & datasets);

    static std::unique_ptr<DataObject> readRawFile(const QString & fileName);


    static vtkSmartPointer<vtkPolyData> parsePoints(const io::InputVector & parsedData, size_t firstColumn);
    static vtkSmartPointer<vtkPolyData> parseIndexedTriangles(
        const io::InputVector & parsedVertexData, size_t vertexIndexColumn, size_t firstVertexColumn,
        const io::InputVector & parsedIndexData, size_t firstIndexColumn);
    static vtkSmartPointer<vtkFloatArray> parseFloatVector(const io::InputVector & parsedData, const QString & arrayName, size_t firstColumn, size_t lastColumn);
};
