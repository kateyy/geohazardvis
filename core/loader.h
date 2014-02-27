#pragma once

#include <memory>
#include <vector>

#include <vtkSmartPointer.h>

#include "common/ebem3d_common.h"

class vtkPolyDataMapper;
class vtkPolyData;

class Input;
class GridDataInput;
class PolyDataInput;
class ProcessedInput;

class Loader
{
public:
    static std::shared_ptr<PolyDataInput> loadIndexedTriangles(const std::string & inputFileName);
    static std::shared_ptr<GridDataInput> loadGrid(const std::string & inputFileName);

protected:
    typedef std::vector<std::vector<t_FP>> InputVector;

protected:
    static InputVector * loadData(const std::string & filename);
    static vtkPolyData * parsePoints(const InputVector & parsedData, t_UInt firstColumn);
    static vtkPolyData * parseIndexedTriangles(
        const InputVector & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const InputVector & parsedIndexData, t_UInt firstIndexColumn);
};
