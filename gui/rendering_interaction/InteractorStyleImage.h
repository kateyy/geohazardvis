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

    void setRenderedData(const QList<RenderedData *> & renderedData) override;

    DataObject * highlightedDataObject() const override;
    vtkIdType highlightedIndex() const override;

    void highlightIndex(DataObject * dataObject, vtkIdType index) override;
    void lookAtIndex(DataObject * dataObject, vtkIdType index) override;

protected:
    explicit InteractorStyleImage();

    void highlightPickedPoint();

    void sendPointInfo() const;

protected:
    QMap<vtkProp *, RenderedData *> m_propToRenderedData;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkActor> m_highlightingActor;
    QPair<DataObject *, vtkIdType> m_currentlyHighlighted;

    bool m_mouseMoved;
};
