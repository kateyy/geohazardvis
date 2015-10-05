#pragma once

#include <vtkType.h>

#include <gui/gui_api.h>


class vtkCamera;

class AbstractVisualizedData;
enum class IndexType;


/** Interface extensions for the vtkInteractorStyle classes 
  * Implement camera movements and reset while ensuring the constraints defined by a specific interaction style. */
class GUI_API ICameraInteractionStyle
{
public:
    virtual void resetCamera(vtkCamera & camera) = 0;
    virtual void moveCameraTo(
        AbstractVisualizedData & visualization, 
        vtkIdType index, IndexType indexType, 
        bool overTime = true) = 0;

    virtual ~ICameraInteractionStyle() = default;
};
