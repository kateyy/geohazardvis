#include "RendererImplementationSwitch.h"

#include <cassert>

#include <QList>

#include <core/data_objects/DataObject.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementation.h>


RendererImplementationSwitch::RendererImplementationSwitch(
    RenderView & renderView)
    : m_view(renderView)
{
    connect(&renderView, &RenderView::resetImplementation, this, &RendererImplementationSwitch::findSuitableImplementation);

    for (const RendererImplementation::ImplementationConstructor & constructor : RendererImplementation::constructors())
    {
        RendererImplementation * instance = constructor(m_view);

        m_implementations.insert(instance->name(), instance);
    }
}

RendererImplementationSwitch::~RendererImplementationSwitch()
{
    m_view.setImplementation(nullptr);
    qDeleteAll(m_implementations.values());
}

void RendererImplementationSwitch::setImplementation(const QString & name)
{
    RendererImplementation * impl = m_implementations.value(name, nullptr);
    if (!impl)
        return;

    m_view.setImplementation(impl);
}

void RendererImplementationSwitch::findSuitableImplementation(const QList<DataObject *> & dataObjects)
{
    RendererImplementation * suitable = nullptr;

    for (RendererImplementation * impl : m_implementations)
    {
        if (impl->canApplyTo(dataObjects))
        {
            suitable = impl;
            break;
        }
    }

    m_view.setImplementation(suitable);
}
