#pragma once

#include <vtkInteractorStyleTrackballCamera.h>

class PickingInteractionStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static PickingInteractionStyle* New();
    vtkTypeMacro(PickingInteractionStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnLeftButtonDown() override;

};
