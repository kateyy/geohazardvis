#include "RenderViewStrategy.h"

#include <vtkCamera.h>

#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/rendering_interaction/CameraInteractorStyleSwitch.h>


RenderViewStrategy::RenderViewStrategy(RendererImplementationBase3D & context)
    : QObject()
    , m_context(context)
{
}

RenderViewStrategy::~RenderViewStrategy() = default;

DataMapping & RenderViewStrategy::dataMapping() const
{
    return m_context.dataMapping();
}

void RenderViewStrategy::activate()
{
    if (!defaultInteractorStyle().isEmpty())
    {
        m_context.interactorStyleSwitch()->setCurrentStyle(defaultInteractorStyle().toStdString());
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
