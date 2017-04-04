#pragma once

#include <type_traits>

#include <vtkCommand.h>

#include <core/core_api.h>


/**
 * User defined events that extent the event list in vtkCommand.
 *
 * Command ids should also be defined here to omit conflicting event id definitions.
 */
class CORE_API vtkCommandExt
{
public:
    enum EventIds : std::underlying_type_t<decltype(vtkCommand::UserEvent)>
    {
        /**
         * Extent vtkCommand::WindowFrameEvent by enforcing an immediate repaint of GUI elements.
         * Useful for example during animation. See also: QWidget:repaint()
         */
        ForceRepaintEvent = vtkCommand::UserEvent + 1
    };
};
