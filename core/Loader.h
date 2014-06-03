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
struct ReadDataset;


class Loader
{
public:
    static std::shared_ptr<Input> readFile(const std::string & filename);

protected:
    static void loadIndexedTriangles(PolyDataInput & input, const std::vector<ReadDataset> & datasets);
    static void loadGrid(GridDataInput & input, const std::vector<ReadDataset> & datasets);

    typedef std::vector<std::vector<t_FP>> InputVector;
    static vtkSmartPointer<vtkPolyData> parsePoints(const InputVector & parsedData, t_UInt firstColumn);
    static vtkSmartPointer<vtkPolyData> parseIndexedTriangles(
        const InputVector & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const InputVector & parsedIndexData, t_UInt firstIndexColumn);
};
