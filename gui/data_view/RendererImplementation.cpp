#include "RendererImplementation.h"


RendererImplementation::RendererImplementation(AbstractRenderView & renderView, QObject * parent)
    : QObject(parent)
    , m_renderView(renderView)
{
}

RendererImplementation::~RendererImplementation()
{
    for (const auto & list : m_visConnections)
    {
        for (auto && c : list)
            disconnect(c);
    }
}

AbstractRenderView & RendererImplementation::renderView() const
{
    return m_renderView;
}

void RendererImplementation::activate(QVTKWidget * /*qvtkWidget*/)
{
}

void RendererImplementation::deactivate(QVTKWidget * /*qvtkWidget*/)
{
    for (const auto & list : m_visConnections)
    {
        for (auto && c : list)
            disconnect(c);
    }
}

void RendererImplementation::addContent(AbstractVisualizedData * content)
{
    onAddContent(content);
}

void RendererImplementation::removeContent(AbstractVisualizedData * content)
{
    auto connectionList = m_visConnections.take(content);
    for (auto && connection : connectionList)
        disconnect(connection);

    onRemoveContent(content);
}

const QList<RendererImplementation::ImplementationConstructor> & RendererImplementation::constructors()
{
    return s_constructors();
}

QList<RendererImplementation::ImplementationConstructor> & RendererImplementation::s_constructors()
{
    static QList<ImplementationConstructor> list;

    return list;
}

void RendererImplementation::addConnectionForContent(AbstractVisualizedData * content,
    const QMetaObject::Connection & connection)
{
    m_visConnections[content] << connection;
}
