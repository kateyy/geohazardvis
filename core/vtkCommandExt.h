/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
