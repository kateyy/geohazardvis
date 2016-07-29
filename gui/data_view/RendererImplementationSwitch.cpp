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
