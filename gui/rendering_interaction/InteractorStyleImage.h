#pragma once

#include <QMap>

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>


class vtkCellPicker;
class vtkProp;


class GUI_API InteractorStyleImage : public IPickingInteractorStyle, public vtkInteractorStyleImage
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
    QMap<vtkProp *, RenderedData *> m_actorToRenderedData;

    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkActor> m_selectedCellActor;

    bool m_mouseMoved;
};
