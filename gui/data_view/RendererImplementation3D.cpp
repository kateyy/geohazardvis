/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RendererImplementation3D.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>


bool RendererImplementation3D::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementation3D>();


RendererImplementation3D::RendererImplementation3D(AbstractRenderView & renderView)
    : RendererImplementationBase3D{ renderView }
    , m_currentStrategy{ nullptr }
{
}

RendererImplementation3D::~RendererImplementation3D()
{
    if (m_currentStrategy)
    {
        m_currentStrategy->deactivate();
    }
}

QString RendererImplementation3D::name() const
{
    return "Renderer 3D";
}

QList<DataObject *> RendererImplementation3D::filterCompatibleObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects)
{
    updateStrategies(dataObjects);

    return strategy().filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementation3D::activate(t_QVTKWidget & qvtkWidget)
{
    RendererImplementationBase3D::activate(qvtkWidget);
}

void RendererImplementation3D::onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    RendererImplementationBase3D::onRemoveContent(content, subViewIndex);

    updateStrategies();
}

void RendererImplementation3D::updateForCurrentInteractionStrategy(const QString & strategyName)
{
    auto it = m_strategies.find(strategyName);
    RenderViewStrategy * newStrategy = it == m_strategies.end()
        ? nullptr
        : it->second.get();

    if (newStrategy == m_currentStrategy)
    {
        return;
    }

    if (m_currentStrategy)
    {
        m_currentStrategy->deactivate();
    }

    m_currentStrategy = newStrategy;

    if (m_currentStrategy)
    {
        m_currentStrategy->activate();
    }
}

RenderViewStrategy * RendererImplementation3D::strategyIfEnabled() const
{
    return m_currentStrategy;
}

void RendererImplementation3D::updateStrategies(const QList<DataObject *> & newDataObjects)
{
    QStringList newNames;

    if (m_strategies.empty())
    {

        for (const RenderViewStrategy::StategyConstructor & constructor : RenderViewStrategy::constructors())
        {
            auto instance = constructor(*this);

            auto && name = instance->name();
            assert(m_strategies.find(name) == m_strategies.end());
            m_strategies.emplace(name, std::move(instance));
            newNames << name;
        }
    }

    bool wasEmpty = m_renderView.visualizations().isEmpty();

    if (wasEmpty)
    {
        // reset the strategy if we were empty

        auto && current = mostSuitableStrategy(newDataObjects);

        if (newNames.isEmpty())
        {
            setInteractionStrategy(current);
        }
        else
        {
            setSupportedInteractionStrategies(newNames, current);
        }
    }
}

QString RendererImplementation3D::mostSuitableStrategy(const QList<DataObject *> & newDataObjects) const
{
    auto && dataObjects = m_renderView.dataObjects() + newDataObjects;

    // use 2D interaction by default, if there is a 2D image in our view
    // viewing 2D images in a 3D terrain view is probably not what we want in most cases

    const bool contains2D = std::any_of(dataObjects.cbegin(), dataObjects.cend(),
        [] (const DataObject * data)
    {
        if (!data->is3D())
        {
            return true;
        }

        return data->bounds().extractDimension(2).componentSize()
            <= std::numeric_limits<float>::epsilon();
    });

    return contains2D ? "2D image" : "3D terrain";
}
