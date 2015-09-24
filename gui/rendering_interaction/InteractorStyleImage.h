#pragma once

#include <memory>

#include <vtkInteractorStyleImage.h>

#include <gui/rendering_interaction/ICameraInteractionStyle.h>


class GUI_API InteractorStyleImage : public vtkInteractorStyleImage, virtual public ICameraInteractionStyle
{
public:
    static InteractorStyleImage * New();
    vtkTypeMacro(InteractorStyleImage, vtkInteractorStyleImage);

    void OnLeftButtonDown() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;

    void OnChar() override;

    void resetCamera() override;
    void moveCameraTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime = true) override;

protected:
    explicit InteractorStyleImage();
    ~InteractorStyleImage() override;
};
