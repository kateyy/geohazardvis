#pragma once

#include <QMap>

#include <vtkInteractorStyleTerrain.h>
#include <vtkSmartPointer.h>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>


class vtkPointPicker;
class vtkCellPicker;
class vtkDataSetMapper;


class GUI_API InteractorStyle3D : public IPickingInteractorStyle, public vtkInteractorStyleTerrain
{
    Q_OBJECT

public:
    static InteractorStyle3D * New();
    vtkTypeMacro(InteractorStyle3D, vtkInteractorStyleTerrain);

    void setRenderedData(QList<RenderedData *> renderedData);

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;

    void OnChar() override;

public slots:
    void highlightCell(DataObject * dataObject, vtkIdType cellId) override;
    void lookAtCell(DataObject * polyData, vtkIdType cellId) override;

protected:
    explicit InteractorStyle3D();

    void MouseWheelDolly(bool forward);

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
