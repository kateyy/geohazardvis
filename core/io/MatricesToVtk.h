#pragma once

#include <core/core_api.h>
#include <core/io/types.h>


class QString;
class vtkPolyData;
class vtkFloatArray;
class DataObject;


class CORE_API MatricesToVtk
{
public:
    static DataObject * loadIndexedTriangles(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static DataObject * loadDEM(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static DataObject * loadGrid2D(const QString & name, const std::vector<io::ReadDataSet> & datasets);
    static DataObject * loadGrid3D(const QString & name, const std::vector<io::ReadDataSet> & datasets);

    static DataObject * readRawFile(const QString & fileName);


    static vtkPolyData * parsePoints(const io::InputVector & parsedData, size_t firstColumn);
    static vtkPolyData * parseIndexedTriangles(
        const io::InputVector & parsedVertexData, size_t vertexIndexColumn, size_t firstVertexColumn,
        const io::InputVector & parsedIndexData, size_t firstIndexColumn);
    static vtkFloatArray * parseFloatVector(const io::InputVector & parsedData, const QString & arrayName, size_t firstColumn, size_t lastColumn);
};
