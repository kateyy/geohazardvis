#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include <QObject>


class vtkPointPicker;
class vtkCellPicker;
class vtkSelection;
class vtkDataSetMapper;
class vtkActor;
class vtkDataObject;
class vtkPolyData;


class PickingInteractionStyle : public QObject, public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT

public:
    explicit PickingInteractionStyle();

    static PickingInteractionStyle* New();
    vtkTypeMacro(PickingInteractionStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnMouseMove() override;
    virtual void OnLeftButtonDown() override;
    virtual void OnLeftButtonUp() override;

public slots:
    void highlightCell(vtkIdType cellId, vtkDataObject * dataObject);
    void lookAtCell(vtkPolyData * polyData, vtkIdType cellId);

signals:
    void pointInfoSent(const QStringList &info) const;
    /** actor at the mouse position, after releasing (and not moving) the left mouse button
        This is only emitted if a actor was picked at the mouse position. */
    void actorPicked(vtkActor * actor);
    void cellPicked(vtkDataObject * dataObject, vtkIdType cellId) const;

protected:
    void pickPoint();
    void pickCell();

    void sendPointInfo() const;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkActor> m_selectedCellActor;
    vtkSmartPointer<vtkDataSetMapper> m_selectedCellMapper;

    bool m_mouseMoved;
};
