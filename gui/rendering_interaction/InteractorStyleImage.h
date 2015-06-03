#pragma once

#include <QMap>

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>


class vtkPointPicker;
class vtkProp;
class vtkActor;


class GUI_API InteractorStyleImage : public IPickingInteractorStyle, public vtkInteractorStyleImage
{
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

public:
    void highlightCell(DataObject * dataObject, vtkIdType cellId) override;
    void lookAtCell(DataObject * dataObject, vtkIdType cellId) override;

protected:
    explicit InteractorStyleImage();

    void highlightPickedPoint();

    void sendPointInfo() const;

protected:
    QMap<vtkProp *, RenderedData *> m_propToRenderedData;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkActor> m_highlightingActor;

    bool m_mouseMoved;
};
