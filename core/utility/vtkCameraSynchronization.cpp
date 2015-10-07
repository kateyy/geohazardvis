#include "vtkCameraSynchronization.h"

#include <cassert>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>


vtkCameraSynchronization::vtkCameraSynchronization()
    : m_isEnabled(true)
    , m_currentlySyncing(false)
{
}

vtkCameraSynchronization::~vtkCameraSynchronization()
{
    while (!m_renderers.isEmpty())
    {
        remove(m_renderers.begin().value());
    }
}

void vtkCameraSynchronization::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void vtkCameraSynchronization::add(vtkRenderer * renderer)
{
    auto camera = renderer->GetActiveCamera();
    if (m_cameras.contains(camera))
        return;

    auto observerTag = camera->AddObserver(vtkCommand::ModifiedEvent, this, &vtkCameraSynchronization::cameraChanged);

    m_cameras.insert(camera, observerTag);
    m_renderers.insert(camera, renderer);
}

void vtkCameraSynchronization::remove(vtkRenderer * renderer)
{
    auto camera = renderer->GetActiveCamera();
    auto it = m_cameras.find(camera);

    if (it == m_cameras.end())
        return;

    camera->RemoveObserver(it.value());

    m_cameras.erase(it);
    m_renderers.remove(camera);
}

void vtkCameraSynchronization::set(const QList<vtkRenderer *> & renderers)
{
    for (auto & renderer : renderers)
        add(renderer);
}

void vtkCameraSynchronization::cameraChanged(vtkObject * source, unsigned long /*event*/, void * /*userData*/)
{
    if (!m_isEnabled || m_currentlySyncing)
        return;

    m_currentlySyncing = true;

    assert(vtkCamera::SafeDownCast(source));
    vtkCamera * sourceCamera = static_cast<vtkCamera *>(source);

    for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it)
    {
        auto & camera = it.key();

        if (camera == sourceCamera)
            continue;

        camera->SetParallelProjection(sourceCamera->GetParallelProjection());
        camera->SetParallelScale(sourceCamera->GetParallelScale());
        camera->SetViewUp(sourceCamera->GetViewUp());
        camera->SetPosition(sourceCamera->GetPosition());
        camera->SetFocalPoint(sourceCamera->GetFocalPoint());
        m_renderers[camera]->ResetCameraClippingRange();
    }

    m_currentlySyncing = false;
}
