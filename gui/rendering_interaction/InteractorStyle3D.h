#pragma once

#include <memory>

#include <vtkInteractorStyleTerrain.h>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>


class vtkProp;

class Highlighter;
class Picker;


class GUI_API InteractorStyle3D : public IPickingInteractorStyle, public vtkInteractorStyleTerrain
{
public:
    static InteractorStyle3D * New();
    vtkTypeMacro(InteractorStyle3D, vtkInteractorStyleTerrain);

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

    DataObject * highlightedDataObject() const override;
    vtkIdType highlightedIndex() const override;

    void highlightIndex(DataObject * dataObject, vtkIdType index) override;
    void lookAtIndex(DataObject * polyData, vtkIdType index) override;
    void flashHightlightedCell(unsigned int milliseconds = 2000u);

protected:
    explicit InteractorStyle3D();
    ~InteractorStyle3D();

    void MouseWheelDolly(bool forward);

    void highlightPickedIndex();

protected:
    std::unique_ptr<Picker> m_picker;
    std::unique_ptr<Highlighter> m_highlighter;

    bool m_mouseMoved;
};
