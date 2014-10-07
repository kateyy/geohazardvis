#include "RenderViewSwitch.h"

#include <cassert>

#include <QList>

#include <core/data_objects/RenderedData.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RenderViewStrategy.h>


RenderViewSwitch::RenderViewSwitch(
    RenderView & renderView)
    : m_view(renderView)
{
    connect(&renderView, &RenderView::renderedDataChanged, this, &RenderViewSwitch::updateStrategies);
    connect(&renderView, &RenderView::resetStrategie, this, &RenderViewSwitch::findSuitableStrategie);

    for (const RenderViewStrategy::StategyConstructor & constructor : RenderViewStrategy::constructors())
    {
        RenderViewStrategy * instance = constructor(m_view);

        m_strategies.insert(instance->name(), instance);
        m_strategieStates.insert(instance->name(), false);
    }
}

RenderViewSwitch::~RenderViewSwitch()
{
    qDeleteAll(m_strategies.values());
}

const QMap<QString, bool> & RenderViewSwitch::applicableStrategies() const
{
    return m_strategieStates;
}

void RenderViewSwitch::setStrategy(const QString & name)
{
    RenderViewStrategy * strategy = m_strategies.value(name, nullptr);
    if (!strategy)
        return;

    if (!m_strategieStates.value(name, false))
        return;

    m_view.setStrategy(strategy);
}

void RenderViewSwitch::updateStrategies(const QList<RenderedData *> & renderedData)
{
    for (auto it = m_strategies.begin(); it != m_strategies.end(); ++it)
    {
        RenderViewStrategy * strategy = it.value();
        
        m_strategieStates.insert(strategy->name(), strategy->canApplyTo(renderedData));
    }

    emit strategiesChanged(m_strategieStates);
}

void RenderViewSwitch::findSuitableStrategie()
{
    for (auto it = m_strategieStates.begin(); it != m_strategieStates.end(); ++it)
    {
        if (!it.value())
            continue;

        RenderViewStrategy * strategy = m_strategies.value(it.key());
        m_view.setStrategy(strategy);
        return;
    }
}
