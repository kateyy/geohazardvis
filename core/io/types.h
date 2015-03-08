#pragma once

#include <string>
#include <vector>

#include <vtkSmartPointer.h>


class vtkDataObject;


namespace io
{

using t_FP = double;
using InputVector = std::vector<std::vector<t_FP>>;

enum class DataSetType
{
    unknown,
    vertices,   // index + vec3
    indices,    // indices referring to a vertex list
    vectors,    // additional vector data per cell
    grid2D,
    vectorGrid3D
};

enum class ModelType
{
    raw,
    triangles,
    DEM,
    grid2D,
    vectorGrid3D
};

struct ReadDataSet
{
    DataSetType type;
    std::vector<std::vector<t_FP>> data;
    std::string attributeName;
    vtkSmartPointer<vtkDataObject> vtkMetaData;
};

}
