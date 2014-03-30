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

class QItemSelection;

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
    void highlightCell(int cellId, vtkDataObject * dataObject);

signals:
    void pointInfoSent(const QStringList &info) const;
    void selectionChanged(int index) const;

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
