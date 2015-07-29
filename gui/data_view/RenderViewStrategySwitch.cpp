#include "RenderViewStrategySwitch.h"

#include <cassert>

#include <QList>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementation3D.h>
#include <gui/data_view/RenderViewStrategy.h>


RenderViewStrategySwitch::RenderViewStrategySwitch(
    RendererImplementation3D & renderViewImpl)
    : QObject()
    , m_view(renderViewImpl)
{
    AbstractRenderView & renderView = renderViewImpl.renderView();
    connect(&renderView, &AbstractRenderView::visualizationsChanged, this, &RenderViewStrategySwitch::updateStrategies);
    connect(&renderViewImpl, &RendererImplementation3D::resetStrategy, this, &RenderViewStrategySwitch::findSuitableStrategy);

    for (const RenderViewStrategy::StategyConstructor & constructor : RenderViewStrategy::constructors())
    {
        auto instance = constructor(m_view);

        QString name = instance->name();
        assert(m_strategies.find(name) == m_strategies.end() && !m_strategyStates.contains(name));
        m_strategies.emplace(name, std::move(instance));
        m_strategyStates.insert(name, false);
    }
}

RenderViewStrategySwitch::~RenderViewStrategySwitch()
{
    m_view.setStrategy(nullptr);
}

const QMap<QString, bool> & RenderViewStrategySwitch::applicableStrategies() const
{
    return m_strategyStates;
}

void RenderViewStrategySwitch::setStrategy(const QString & name)
{
    auto it = m_strategies.find(name);
    if (it == m_strategies.end())
        return;

    if (!m_strategyStates.value(name, false))
        return;

    m_view.setStrategy(it->second.get());
}

void RenderViewStrategySwitch::updateStrategies()
{
    const QList<RenderedData *> & renderedData = m_view.renderedData();

    for (auto & pair : m_strategies)
    {
        RenderViewStrategy * strategy = pair.second.get();
        
        m_strategyStates.insert(strategy->name(), strategy->canApplyTo(renderedData));
    }

    emit strategiesChanged(m_strategyStates);
}

void RenderViewStrategySwitch::findSuitableStrategy(const QList<DataObject *> & dataObjects)
{
    int mostCompatible = 0;
    RenderViewStrategy * mostSuitable = nullptr;
    for (auto it = m_strategyStates.begin(); it != m_strategyStates.end(); ++it)
    {
        auto strategyIt = m_strategies.find(it.key());
        assert(strategyIt != m_strategies.end());
        QList<DataObject *> incompatible;
        QList<DataObject *> list = strategyIt->second->filterCompatibleObjects(dataObjects, incompatible);
        
        if (list.length() > mostCompatible)
        {
            mostCompatible = list.length();
            mostSuitable = strategyIt->second.get();
        }
    }

    m_view.setStrategy(mostSuitable);
}
