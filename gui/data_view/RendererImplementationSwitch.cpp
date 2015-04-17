#include "RendererImplementationSwitch.h"

#include <cassert>

#include <gui/data_view/RendererImplementationNull.h>


RendererImplementationSwitch::RendererImplementationSwitch(RenderView & renderView)
    : m_view(renderView)
    , m_currentImpl(nullptr)
    , m_nullImpl(nullptr)
{
}

RendererImplementationSwitch::~RendererImplementationSwitch()
{
    delete m_currentImpl;
    delete m_nullImpl;
}

void RendererImplementationSwitch::findSuitableImplementation(const QList<DataObject *> & dataObjects)
{
    RendererImplementation * suitable = nullptr;

    for (const RendererImplementation::ImplementationConstructor & constructor : RendererImplementation::constructors())
    {
        RendererImplementation * instance = constructor(m_view);

        if (instance->canApplyTo(dataObjects))
        {
            suitable = instance;
            break;
        }
    }

    auto lastImpl = m_currentImpl;
    m_currentImpl = suitable;
    delete lastImpl;
}

RendererImplementation & RendererImplementationSwitch::currentImplementation()
{
    if (m_currentImpl)
        return *m_currentImpl;

    if (!m_nullImpl)
        m_nullImpl = new RendererImplementationNull(m_view);

    return *m_nullImpl;
}
