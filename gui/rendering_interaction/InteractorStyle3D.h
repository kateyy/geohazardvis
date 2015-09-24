#pragma once

#include <memory>

#include <vtkInteractorStyleTerrain.h>

#include <gui/rendering_interaction/ICameraInteractionStyle.h>


class CameraDolly;


class GUI_API InteractorStyle3D : public vtkInteractorStyleTerrain, virtual public ICameraInteractionStyle
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

    void resetCamera() override;
    void moveCameraTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime = true) override;

protected:
    explicit InteractorStyle3D();
    ~InteractorStyle3D();

    void MouseWheelDolly(bool forward);

private:
    std::unique_ptr<CameraDolly> m_cameraDolly;

    bool m_mouseMoved;
};
