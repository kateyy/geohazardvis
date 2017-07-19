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

#include "RenderViewStrategy.h"

#include <vtkCamera.h>

#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/rendering_interaction/CameraInteractorStyleSwitch.h>


RenderViewStrategy::RenderViewStrategy(RendererImplementationBase3D & context)
    : QObject()
    , m_context{ context }
{
}

RenderViewStrategy::~RenderViewStrategy() = default;

DataMapping & RenderViewStrategy::dataMapping() const
{
    return m_context.dataMapping();
}

void RenderViewStrategy::activate()
{
    const auto style = defaultInteractorStyle();
    if (!style.isEmpty())
    {
        m_context.interactorStyleSwitch()->setCurrentStyle(style.toStdString());
    }

    restoreCamera();

    onActivateEvent();
}

void RenderViewStrategy::deactivate()
{
    backupCamera();

    onDeactivateEvent();
}

const std::vector<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::constructors()
{
    return s_constructors();
}

QString RenderViewStrategy::defaultInteractorStyle() const
{
    return QString();
}

void RenderViewStrategy::onActivateEvent()
{
}

void RenderViewStrategy::onDeactivateEvent()
{
}

void RenderViewStrategy::restoreCamera()
{
    auto & camera = *m_context.camera(0);
    if (m_storedCamera)
    {
        camera.DeepCopy(m_storedCamera);
    }
    else
    {
        m_context.interactorStyleSwitch()->resetCameraToDefault(camera);
    }

    m_context.resetClippingRanges();
}

void RenderViewStrategy::backupCamera()
{
    if (!m_storedCamera)
    {
        m_storedCamera = vtkSmartPointer<vtkCamera>::New();
    }

    m_storedCamera->DeepCopy(m_context.camera(0));
}

std::vector<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::s_constructors()
{
    static std::vector<StategyConstructor> list;

    return list;
}
