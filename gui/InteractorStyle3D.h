#pragma once

#include <QList>

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include "IPickingInteractorStyle.h"


class vtkPointPicker;
class vtkCellPicker;
class vtkDataSetMapper;


class InteractorStyle3D : public IPickingInteractorStyle, public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT

public:
    static InteractorStyle3D * New();
    vtkTypeMacro(InteractorStyle3D, vtkInteractorStyleTrackballCamera);

    virtual void OnMouseMove() override;
    virtual void OnLeftButtonDown() override;
    virtual void OnLeftButtonUp() override;

    void setRenderedDataList(const QList<RenderedData *> * renderedData);

public slots:
    void highlightCell(vtkIdType cellId, DataObject * dataObject) override;
    void lookAtCell(vtkPolyData * polyData, vtkIdType cellId) override;

protected:
    explicit InteractorStyle3D();

    void pickPoint();
    void pickCell();

    void sendPointInfo() const;

protected:
    const QList<RenderedData *> * m_renderedData;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkActor> m_selectedCellActor;
    vtkSmartPointer<vtkDataSetMapper> m_selectedCellMapper;

    bool m_mouseMoved;
};
