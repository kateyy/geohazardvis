#pragma once

#include <vector>

#include <QString>

#include <core/common/ebem3d_common.h>


class vtkPolyData;
class vtkFloatArray;
class DataObject;
struct ReadDataset;


class MatricesToVtk
{
public:
    static DataObject * loadIndexedTriangles(QString name, const std::vector<ReadDataset> & datasets);
    static DataObject * loadDEM(QString name, const std::vector<ReadDataset> & datasets);
    static DataObject * loadGrid2D(QString name, const std::vector<ReadDataset> & datasets);
    static DataObject * loadGrid3D(QString name, const std::vector<ReadDataset> & datasets);

    static DataObject * readRawFile(QString fileName);

    typedef std::vector<std::vector<t_FP>> InputVector;
    static vtkPolyData * parsePoints(const InputVector & parsedData, t_UInt firstColumn);
    static vtkPolyData * parseIndexedTriangles(
        const InputVector & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const InputVector & parsedIndexData, t_UInt firstIndexColumn);
    static vtkFloatArray * parseFloatVector(const InputVector & parsedData, QString arrayName, int firstColumn, int lastColumn);
};
