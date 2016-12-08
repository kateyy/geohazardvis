#include "RendererImplementation.h"

#include <cassert>

#include <core/AbstractVisualizedData.h>
#include <core/utility/qthelper.h>
#include <gui/data_view/AbstractRenderView.h>


RendererImplementation::RendererImplementation(AbstractRenderView & renderView)
    : QObject()
    , m_renderView{ renderView }
{
}

RendererImplementation::~RendererImplementation()
{
    for (auto & listIt : m_visConnections)
    {
        disconnectAll(listIt.second);
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

void RendererImplementation::applyCurrentCoordinateSystem(const CoordinateSystemSpecification & /*spec*/)
{
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

void RendererImplementation::activate(t_QVTKWidget & /*qvtkWidget*/)
{
}

void RendererImplementation::deactivate(t_QVTKWidget & /*qvtkWidget*/)
{
    for (auto & listIt : m_visConnections)
    {
        disconnectAll(listIt.second);
    }
    m_visConnections.clear();
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
    auto it = m_visConnections.find(content);
    assert(it != m_visConnections.end());
    disconnectAll(it->second);
    m_visConnections.erase(it);

    onRemoveContent(content, subViewIndex);

    disconnect(content, &AbstractVisualizedData::geometryChanged, this, &RendererImplementation::render);
}

void RendererImplementation::renderViewContentsChanged()
{
    onRenderViewContentsChanged();
}

void RendererImplementation::setSelection(const VisualizationSelection & selection)
{
    // This is either called by the view (setVisualizationSelection etc)
    // or by the picking/highlighting implementations.

    if (selection == m_selection)
    {
        return;
    }

    m_selection = selection;

    onSetSelection(m_selection);

    m_renderView.setVisualizationSelection(selection);
}

void RendererImplementation::clearSelection()
{
    if (m_selection.isEmpty())
    {
        return;
    }

    m_selection.clear();

    onClearSelection();

    m_renderView.clearSelection();
}

const VisualizationSelection & RendererImplementation::selection() const
{
    return m_selection;
}

const std::vector<RendererImplementation::ImplementationConstructor> & RendererImplementation::constructors()
{
    return s_constructors();
}

std::vector<RendererImplementation::ImplementationConstructor> & RendererImplementation::s_constructors()
{
    static std::vector<ImplementationConstructor> list;

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
    m_visConnections[content].emplace_back(connection);
}
