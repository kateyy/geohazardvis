#pragma once

#include <QMap>

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

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;

    void OnChar() override;

    void setRenderedData(QList<RenderedData *> renderedData);

public slots:
    void highlightCell(DataObject * dataObject, vtkIdType cellId) override;
    void lookAtCell(DataObject * polyData, vtkIdType cellId) override;

protected:
    explicit InteractorStyle3D();

    void highlightPickedCell();
    void sendPointInfo() const;

protected:
    QMap<vtkActor *, RenderedData *> m_actorToRenderedData;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkActor> m_selectedCellActor;
    vtkSmartPointer<vtkDataSetMapper> m_selectedCellMapper;

    bool m_mouseMoved;
};
