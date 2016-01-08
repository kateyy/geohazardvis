#include "RendererImplementation.h"

#include <cassert>

#include <core/AbstractVisualizedData.h>
#include <gui/data_view/AbstractRenderView.h>


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

DataMapping & RendererImplementation::dataMapping() const
{
    return m_renderView.dataMapping();
}

const QStringList & RendererImplementation::supportedInteractionStrategies() const
{
    return m_supportedInteractionStrategies;
}

void RendererImplementation::setInteractionStrategy(const QString & strategyName)
{
    if (m_currentInteractionStrategy == strategyName)
    {
        return;
    }

    m_currentInteractionStrategy = strategyName;

    updateForCurrentInteractionStrategy(strategyName);

    emit interactionStrategyChanged(strategyName);
}

const QString & RendererImplementation::currentInteractionStrategy() const
{
    return m_currentInteractionStrategy;
}

void RendererImplementation::activate(QVTKWidget & /*qvtkWidget*/)
{
}

void RendererImplementation::deactivate(QVTKWidget & /*qvtkWidget*/)
{
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
    connect(content, &AbstractVisualizedData::geometryChanged, this, &RendererImplementation::render);

    onAddContent(content, subViewIndex);
}

void RendererImplementation::removeContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    auto connectionList = m_visConnections.take(content);
    for (auto && connection : connectionList)
        disconnect(connection);

    onRemoveContent(content, subViewIndex);

    disconnect(content, &AbstractVisualizedData::geometryChanged, this, &RendererImplementation::render);
}

void RendererImplementation::renderViewContentsChanged()
{
    onRenderViewContentsChanged();
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

void RendererImplementation::onRenderViewContentsChanged()
{
}

void RendererImplementation::setSupportedInteractionStrategies(const QStringList & strategyNames, const QString & currentStrategy)
{
    m_supportedInteractionStrategies = strategyNames;
    m_currentInteractionStrategy = currentStrategy;

    // explicitly call the updates for the current strategy here
    // ensure, to first call internal updates, and after that the update signals

    updateForCurrentInteractionStrategy(currentStrategy);

    emit supportedInteractionStrategiesChanged(m_supportedInteractionStrategies);
    emit interactionStrategyChanged(m_currentInteractionStrategy);
}

void RendererImplementation::updateForCurrentInteractionStrategy(const QString & /*strategyName*/)
{
}

void RendererImplementation::addConnectionForContent(AbstractVisualizedData * content,
    const QMetaObject::Connection & connection)
{
    m_visConnections[content] << connection;
}
