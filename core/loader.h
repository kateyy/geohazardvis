#pragma once

#include <memory>
#include <vector>

#include <vtkSmartPointer.h>

#include "common/ebem3d_common.h"

class vtkPolyDataMapper;
class vtkPolyData;
class vtkActor;
class vtkPolyDataAlgorithm;


struct Input {
    vtkSmartPointer<vtkPolyData> polyData;

    virtual vtkSmartPointer<vtkPolyDataMapper> dataMapper();

    vtkSmartPointer<vtkActor> createActor();

protected:
    vtkSmartPointer<vtkPolyDataMapper> m_dataMapper;
};

struct ProcessedInput : public Input {
    vtkSmartPointer<vtkPolyDataAlgorithm> algorithm;
    virtual vtkSmartPointer<vtkPolyDataMapper> dataMapper() override;
};

class Loader
{
protected:
    typedef std::vector<std::vector<t_FP>> ParsedData;
public:
    static std::shared_ptr<Input> loadFileAsPoints(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);
    static std::shared_ptr<ProcessedInput> loadFileTriangulated(const std::string & filename, t_UInt nbColumns, t_UInt firstToTake);
    static std::shared_ptr<Input> loadIndexedTriangles(
        const std::string & vertexFilename, t_UInt nbVertexFileColumns, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const std::string & indexFilename, t_UInt nbIndexFileColumns, t_UInt firstIndexColumn);

protected:
    static ParsedData * loadData(const std::string & filename, t_UInt nbColumns);
    static vtkSmartPointer<vtkPolyData> parsePoints(const ParsedData & parsedData, t_UInt firstColumn);
    static vtkSmartPointer<vtkPolyData> parseIndexedTriangles(
        const ParsedData & parsedVertexData, t_UInt vertexIndexColumn, t_UInt firstVertexColumn,
        const ParsedData & parsedIndexData, t_UInt firstIndexColumn);
};
