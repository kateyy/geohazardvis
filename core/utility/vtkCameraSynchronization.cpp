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

#include "vtkCameraSynchronization.h"

#include <cassert>

#include <QDebug>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>


vtkCameraSynchronization::vtkCameraSynchronization()
    : m_isEnabled{ true }
    , m_currentlySyncing{ false }
{
}

vtkCameraSynchronization::~vtkCameraSynchronization()
{
    clear();
}

void vtkCameraSynchronization::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void vtkCameraSynchronization::add(vtkRenderer * renderer)
{
    if (!renderer)
    {
        return;
    }

    auto camera = renderer->GetActiveCamera();
    if (m_cameras.find(camera) != m_cameras.end())
    {
        return;
    }

    auto observerTag = camera->AddObserver(vtkCommand::ModifiedEvent, this, &vtkCameraSynchronization::cameraChanged);

    m_cameras.emplace(camera, observerTag);
    m_renderers.emplace(camera, renderer);
}

void vtkCameraSynchronization::remove(vtkRenderer * renderer)
{
    if (!renderer)
    {
        qWarning() << "Passed null renderer to vtkCameraSynchronization";
        return;
    }

    auto camera = renderer->GetActiveCamera();
    auto it = m_cameras.find(camera);

    if (it == m_cameras.end())
    {
        return;
    }

    camera->RemoveObserver(it->second);

    m_cameras.erase(it);
    m_renderers.erase(camera);
}

void vtkCameraSynchronization::set(const std::vector<vtkRenderer *> & renderers)
{
    clear();

    for (const auto & renderer : renderers)
    {
        add(renderer);
    }
}

void vtkCameraSynchronization::clear()
{
    auto oldRenderers = m_renderers;
    for (const auto & renderer : oldRenderers)
    {
        remove(renderer.second);
    }
}

void vtkCameraSynchronization::cameraChanged(vtkObject * source, unsigned long /*event*/, void * /*userData*/)
{
    if (!m_isEnabled || m_currentlySyncing)
    {
        return;
    }

    m_currentlySyncing = true;

    assert(vtkCamera::SafeDownCast(source));
    vtkCamera * sourceCamera = static_cast<vtkCamera *>(source);

    for (const auto & camIt : m_cameras)
    {
        auto & camera = camIt.first;

        if (camera == sourceCamera)
        {
            continue;
        }

        camera->SetParallelProjection(sourceCamera->GetParallelProjection());
        camera->SetParallelScale(sourceCamera->GetParallelScale());
        camera->SetViewUp(sourceCamera->GetViewUp());
        camera->SetPosition(sourceCamera->GetPosition());
        camera->SetFocalPoint(sourceCamera->GetFocalPoint());
        m_renderers[camera]->ResetCameraClippingRange();
    }

    m_currentlySyncing = false;
}
