#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include "gui/pickinginfo.h"

class PickingInteractionStyle : public vtkInteractorStyleTrackballCamera
{
public:
    explicit PickingInteractionStyle();

    static PickingInteractionStyle* New();
    vtkTypeMacro(PickingInteractionStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnMouseMove() override;
    virtual void OnLeftButtonDown() override;

    const PickingInfo pickingInfo; // qt object used to create qt events for vtk pick events

protected:
    void pick(bool mouseClick);
};
