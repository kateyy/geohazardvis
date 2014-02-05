#pragma once

#include <memory>
#include <vector>

#include <vtkSmartPointer.h>

#include "common/ebem3d_common.h"

class vtkPolyDataMapper;
class vtkPolyData;
class vtkActor;
class vtkPolyDataAlgorithm;

class Input;
class DataSetInput;
class PolyDataInput;
class ProcessedInput;
class Context2DInput;

class Loader
{
protected:
    typedef std::vector<std::vector<t_FP>> ParsedData;
public:
    static std::shared_ptr<PolyDataInput> loadFileAsPoints(const std::string & filename, t_UInt firstDataColumn);
    static std::shared_ptr<ProcessedInput> loadFileTriangulated(const std::string & filename, t_UInt firstDataColumn);
    static std::shared_ptr<PolyDataInput> loadIndexedTriangles(
        const std::string & vertexFilename, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const std::string & indexFilename, t_UInt firstIndexColumn);
    static std::shared_ptr<DataSetInput> loadGrid(const std::string & gridFilename, const std::string & xFilename, const std::string & yFilename);
    static std::shared_ptr<Context2DInput> loadGrid2DScene(const std::string & gridFilename, const std::string & xFilename, const std::string & yFilename);

protected:
    static ParsedData * loadData(const std::string & filename);
    static vtkPolyData * parsePoints(const ParsedData & parsedData, t_UInt firstColumn);
    static vtkPolyData * parseIndexedTriangles(
        const ParsedData & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const ParsedData & parsedIndexData, t_UInt firstIndexColumn);
};
