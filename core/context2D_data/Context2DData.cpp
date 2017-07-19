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

#include "Context2DData.h"

#include <cassert>

#include <vtkPlot.h>

#include <core/types.h>
#include <core/context2D_data/vtkPlotCollection.h>


Context2DData::Context2DData(DataObject & dataObject)
    : AbstractVisualizedData(ContentType::Context2D, dataObject)
    , m_plotsInvalidated{ true }
{
}

Context2DData::~Context2DData() = default;

const vtkSmartPointer<vtkPlotCollection> & Context2DData::plots()
{
    if (m_plotsInvalidated)
    {
        m_plotsInvalidated = false;
        m_plots = fetchPlots();
    }

    assert(m_plots);
    assert(m_plots->GetNumberOfItems() > 0);

    return m_plots;
}

void Context2DData::visibilityChangedEvent(bool visible)
{
    vtkCollectionSimpleIterator it;
    plots()->InitTraversal(it);
    while (auto item = plots()->GetNextPlot(it))
    {
        item->SetVisible(visible);
    }
}

void Context2DData::invalidateContextItems()
{
    m_plotsInvalidated = true;

    emit plotCollectionChanged();
}
