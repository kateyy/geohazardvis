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
