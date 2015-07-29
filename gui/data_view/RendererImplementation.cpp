#include "RendererImplementation.h"

#include <cassert>

#include <data_view/AbstractRenderView.h>


RendererImplementation::RendererImplementation(AbstractRenderView & renderView)
    : QObject()
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
    connect(&m_renderView, &AbstractRenderView::visualizationsChanged, this, &RendererImplementation::onRenderViewVisualizationChanged);
}

void RendererImplementation::deactivate(QVTKWidget * /*qvtkWidget*/)
{
    disconnect(&m_renderView, &AbstractRenderView::visualizationsChanged, this, &RendererImplementation::onRenderViewVisualizationChanged);

    for (const auto & list : m_visConnections)
    {
        for (auto && c : list)
            disconnect(c);
    }
}

unsigned int RendererImplementation::subViewIndexAtPos(const QPoint /*pixelCoordinate*/) const
{
    // override this, if it is not trivial
    assert(m_renderView.numberOfSubViews() == 1u);
    return 0u;
}

void RendererImplementation::addContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    onAddContent(content, subViewIndex);
}

void RendererImplementation::removeContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    auto connectionList = m_visConnections.take(content);
    for (auto && connection : connectionList)
        disconnect(connection);

    onRemoveContent(content, subViewIndex);
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

void RendererImplementation::onRenderViewVisualizationChanged()
{
}

void RendererImplementation::addConnectionForContent(AbstractVisualizedData * content,
    const QMetaObject::Connection & connection)
{
    m_visConnections[content] << connection;
}
