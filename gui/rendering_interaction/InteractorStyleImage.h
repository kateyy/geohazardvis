#pragma once

#include <QMap>

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>

#include "IPickingInteractorStyle.h"


class vtkPointPicker;
class vtkCellPicker;
class vtkDataSetMapper;


class InteractorStyleImage : public IPickingInteractorStyle, public vtkInteractorStyleImage
{
    Q_OBJECT

public:
    static InteractorStyleImage * New();
    vtkTypeMacro(InteractorStyleImage, vtkInteractorStyleImage);

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;

    void OnChar() override;

    void setRenderedData(QList<RenderedData *> renderedData) override;

public slots:
    void highlightCell(DataObject * dataObject, vtkIdType cellId) override;
    void lookAtCell(DataObject * dataObject, vtkIdType cellId) override;

protected:
    explicit InteractorStyleImage();

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
