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

#include "RendererImplementationSwitch.h"

#include <cassert>

#include <gui/data_view/RendererImplementationNull.h>


RendererImplementationSwitch::RendererImplementationSwitch(AbstractRenderView & renderView)
    : m_view(renderView)
    , m_currentImpl{ nullptr }
    , m_nullImpl{ nullptr }
{
}

RendererImplementationSwitch::~RendererImplementationSwitch() = default;

void RendererImplementationSwitch::findSuitableImplementation(const QList<DataObject *> & dataObjects)
{
    for (const RendererImplementation::ImplementationConstructor & constructor : RendererImplementation::constructors())
    {
        auto instance = constructor(m_view);

        if (instance->canApplyTo(dataObjects))
        {
            m_currentImpl = std::move(instance);
            break;
        }
    }
}

RendererImplementation & RendererImplementationSwitch::currentImplementation()
{
    if (m_currentImpl)
    {
        return *m_currentImpl;
    }

    if (!m_nullImpl)
    {
        m_nullImpl = std::make_unique<RendererImplementationNull>(m_view);
    }

    return *m_nullImpl;
}
