#pragma once

#include <core/types.h>

#include <gui/gui_api.h>


class vtkCamera;


/** Interface extensions for the vtkInteractorStyle classes 
  * Implement camera movements and reset while ensuring the constraints defined by a specific interaction style. */
class GUI_API ICameraInteractionStyle
{
public:
    virtual void resetCameraToDefault(vtkCamera & camera) = 0;
    virtual void moveCameraTo(
        const VisualizationSelection & selection,
        bool overTime = true) = 0;

    virtual ~ICameraInteractionStyle() = default;
};
