#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <QList>


class vtkPointPicker;
class vtkCellPicker;
class vtkSelection;
class vtkDataSetMapper;
class vtkActor;
class vtkDataObject;
class vtkPolyData;

class DataObject;
class RenderedData;


class PickingInteractionStyle : public QObject, public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT

public:
    static PickingInteractionStyle* New();
    vtkTypeMacro(PickingInteractionStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnMouseMove() override;
    virtual void OnLeftButtonDown() override;
    virtual void OnLeftButtonUp() override;

    void setRenderedDataList(const QList<RenderedData *> * renderedData);

public slots:
    void highlightCell(vtkIdType cellId, DataObject * dataObject);
    void lookAtCell(vtkPolyData * polyData, vtkIdType cellId);

signals:
    void pointInfoSent(const QStringList &info) const;
    /** actor at the mouse position, after releasing (and not moving) the left mouse button
        This is only emitted if a actor was picked at the mouse position. */
    void actorPicked(vtkActor * actor);
    void cellPicked(DataObject * dataObject, vtkIdType cellId) const;

protected:
    explicit PickingInteractionStyle();

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
