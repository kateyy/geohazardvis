#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include "gui/pickinginfo.h"

class Viewer;

class PickingInteractionStyle : public vtkInteractorStyleTrackballCamera
{
public:
    explicit PickingInteractionStyle();

    static PickingInteractionStyle* New();
    vtkTypeMacro(PickingInteractionStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnLeftButtonDown() override;
   
    void setViewer(const Viewer & viewer);

protected:
    const Viewer * m_viewer;

    const PickingInfo m_pickingInfo; // qt object used to create qt events for vtk pick events
};
