#pragma once

#include <memory>

#include <core/core_api.h>
#include <core/io/types.h>


class QString;
class vtkDataArray;
class vtkImageData;
class vtkPolyData;
class DataObject;


class CORE_API MatricesToVtk
{
public:
    static std::unique_ptr<DataObject> loadIndexedTriangles(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadGrid2D(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static std::unique_ptr<DataObject> loadGrid3D(const QString & name, const std::vector<io::ReadDataSet> & datasets);

    static std::unique_ptr<DataObject> readRawFile(const QString & fileName);

    static vtkSmartPointer<vtkImageData> parseGrid2D(const io::InputVector & inputData, int vtk_dataType = VTK_FLOAT);
    static vtkSmartPointer<vtkPolyData> parsePoints(const io::InputVector & inputData, size_t firstColumn,
        int vtk_dataType = VTK_FLOAT);
    static vtkSmartPointer<vtkPolyData> parseIndexedTriangles(
        const io::InputVector & inputVertexData, size_t vertexIndexColumn, size_t firstVertexColumn,
        const io::InputVector & inputIndexData, size_t firstIndexColumn, 
        int vtk_dataType = VTK_FLOAT);
    static vtkSmartPointer<vtkDataArray> parseFloatVector(
        const io::InputVector & inputData, const QString & arrayName, size_t firstColumn, size_t lastColumn,
        int vtk_dataType = VTK_FLOAT);

private:
    MatricesToVtk() = delete;
    ~MatricesToVtk() = delete;
};
