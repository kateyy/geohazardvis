#pragma once

#include <memory>

#include <vtkInteractorStyleImage.h>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>


class Highlighter;
class Picker;


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

    DataObject * highlightedDataObject() const override;
    vtkIdType highlightedIndex() const override;

    void highlightIndex(DataObject * dataObject, vtkIdType index) override;
    void lookAtIndex(DataObject * dataObject, vtkIdType index) override;

protected:
    explicit InteractorStyleImage();
    ~InteractorStyleImage() override;

    void highlightPickedPoint();

protected:
    std::unique_ptr<Picker> m_picker;
    std::unique_ptr<Highlighter> m_highlighter;

    bool m_mouseMoved;
};
