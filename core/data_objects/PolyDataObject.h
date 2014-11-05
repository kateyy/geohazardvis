#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class vtkPolyData;
class vtkPolyDataNormals;
class vtkCellCenters;
class vtkAlgorithmOutput;


class CORE_API PolyDataObject : public DataObject
{
public:
    PolyDataObject(QString name, vtkPolyData * dataSet);

    bool is3D() const override;

    /** @return poly data set with cell normals */
    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;

    void addDataArray(vtkDataArray * dataArray) override;

    /** @return centroids with normals, computed from polygonal data set cells */
    vtkPolyData * cellCenters();
    vtkAlgorithmOutput * cellCentersOutputPort();

    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    bool setCellCenterComponent(vtkIdType cellId, int component, double value);
    bool setCellNormalComponent(vtkIdType cellId, int component, double value);

protected:
    QVtkTableModel * createTableModel() override;

    vtkSmartPointer<vtkPolyDataNormals> m_cellNormals;
    vtkSmartPointer<vtkCellCenters> m_cellCenters;
};
