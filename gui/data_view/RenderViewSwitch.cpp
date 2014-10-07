#include "RenderViewSwitch.h"

#include <cassert>

#include <QList>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RenderViewStrategy.h>


RenderViewSwitch::RenderViewSwitch(
    RenderView & renderView)
    : m_view(renderView)
{
    connect(&renderView, &RenderView::renderedDataChanged, this, &RenderViewSwitch::updateStrategies);
    connect(&renderView, &RenderView::resetStrategie, this, &RenderViewSwitch::findSuitableStrategy);

    for (const RenderViewStrategy::StategyConstructor & constructor : RenderViewStrategy::constructors())
    {
        RenderViewStrategy * instance = constructor(m_view);

        m_strategies.insert(instance->name(), instance);
        m_strategyStates.insert(instance->name(), false);
    }
}

RenderViewSwitch::~RenderViewSwitch()
{
    qDeleteAll(m_strategies.values());
}

const QMap<QString, bool> & RenderViewSwitch::applicableStrategies() const
{
    return m_strategyStates;
}

void RenderViewSwitch::setStrategy(const QString & name)
{
    RenderViewStrategy * strategy = m_strategies.value(name, nullptr);
    if (!strategy)
        return;

    if (!m_strategyStates.value(name, false))
        return;

    m_view.setStrategy(strategy);
}

void RenderViewSwitch::updateStrategies(const QList<RenderedData *> & renderedData)
{
    for (auto it = m_strategies.begin(); it != m_strategies.end(); ++it)
    {
        RenderViewStrategy * strategy = it.value();
        
        m_strategyStates.insert(strategy->name(), strategy->canApplyTo(renderedData));
    }

    emit strategiesChanged(m_strategyStates);
}

void RenderViewSwitch::findSuitableStrategy(const QList<DataObject *> & dataObjects)
{
    int mostCompatible = 0;
    RenderViewStrategy * mostSuitable = nullptr;
    for (auto it = m_strategyStates.begin(); it != m_strategyStates.end(); ++it)
    {
        QList<DataObject *> list = dataObjects;

        RenderViewStrategy * strategy = m_strategies.value(it.key());
        strategy->checkCompatibleObjects(list);
        
        if (list.length() > mostCompatible)
        {
            mostCompatible = list.length();
            mostSuitable = strategy;
        }
    }

    m_view.setStrategy(mostSuitable);
}
