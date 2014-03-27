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

    void setMainDataObject(vtkDataObject * dataObject);

public slots:
    void changeSelection(const QItemSelection & selected, const QItemSelection & deselected);

signals:
    void pointInfoSent(const QStringList &info) const;
    void selectionChanged(int index) const;

protected:
    void pickPoint();
    void pickCell();

    void highlightCell(int cellId, vtkDataObject * dataObject);

    void sendPointInfo() const;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkActor> m_selectedCellActor;
    vtkSmartPointer<vtkDataSetMapper> m_selectedCellMapper;

    vtkSmartPointer<vtkDataObject> m_mainDataObject;
};
