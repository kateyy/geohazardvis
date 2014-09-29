#pragma once

#include <core/data_objects/DataObject.h>
#include <core/core_api.h>


class vtkPolyData;
class vtkCellCenters;
class vtkAlgorithmOutput;


class CORE_API PolyDataObject : public DataObject
{
public:
    PolyDataObject(QString name, vtkPolyData * dataSet);

    bool is3D() const override;

    vtkPolyData * cellCenters();
    vtkAlgorithmOutput * cellCentersOutputPort();

    RenderedData * createRendered() override;

    QString dataTypeName() const override;

protected:
    QVtkTableModel * createTableModel() override;

    vtkSmartPointer<vtkCellCenters> m_cellCenters;
};
