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

#include "RendererImplementationPlot.h"

#include <cassert>

#include <vtkAxis.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkIdTypeArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPlot.h>
#include <vtkRendererCollection.h>

#include <core/data_objects/DataObject.h>
#include <core/context2D_data/Context2DData.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <core/utility/font.h>
#include <core/utility/macros.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/ChartXY.h>
#include <gui/data_view/t_QVTKWidget.h>


bool RendererImplementationPlot::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementationPlot>();


RendererImplementationPlot::RendererImplementationPlot(AbstractRenderView & renderView)
    : RendererImplementation(renderView)
    , m_isInitialized{ false }
    , m_axesAutoUpdate{ true }
    , m_selectedPlot{ nullptr }
{
    assert(renderView.numberOfSubViews() == 1); // multi view not implemented yet. Is that even possible with the vtkContextView?
}

RendererImplementationPlot::~RendererImplementationPlot() = default;

QString RendererImplementationPlot::name() const
{
    return "Context View";
}

ContentType RendererImplementationPlot::contentType() const
{
    return ContentType::Context2D;
}

bool RendererImplementationPlot::canApplyTo(const QList<DataObject *> & dataObjects)
{
    for (auto dataObject : dataObjects)
    {
        if (auto && cd = dataObject->createContextData())
        {
            return true;
        }
    }

    return false;
}

QList<DataObject *> RendererImplementationPlot::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects)
{
    QList<DataObject *> compatible;

    for (auto dataObject : dataObjects)
    {
        if (dataObject->dataTypeName() == "Data Set Profile (2D)") // hard-coded for now
        {
            compatible << dataObject;
        }
        else
        {
            incompatibleObjects << dataObject;
        }
    }

    return compatible;
}

void RendererImplementationPlot::activate(t_QVTKWidget & qvtkWidget)
{
    initialize();

    // see also: vtkRenderViewBase documentation
    m_contextView->SetInteractor(qvtkWidget.GetInteractorBase());
    qvtkWidget.SetRenderWindow(m_renderWindow);
}

void RendererImplementationPlot::deactivate(t_QVTKWidget & qvtkWidget)
{
    // the render window belongs to the context view
    qvtkWidget.SetRenderWindow(nullptr);
    // the interactor is provided by the qvtkWidget
    m_contextView->SetInteractor(nullptr);
}

void RendererImplementationPlot::render()
{
    if (!m_renderView.isVisible())
    {
        return;
    }

    m_contextView->Render();
}

vtkRenderWindowInteractor * RendererImplementationPlot::interactor()
{
    return m_contextView->GetInteractor();
}

void RendererImplementationPlot::lookAtData(const VisualizationSelection & /*selection*/, unsigned int /*subViewIndex*/)
{
}

void RendererImplementationPlot::resetCamera(bool /*toInitialPosition*/, unsigned int /*subViewIndex*/)
{
    m_contextView->ResetCamera();

    render();
}

void RendererImplementationPlot::onAddContent(AbstractVisualizedData * content, unsigned int /*subViewIndex*/)
{
    assert(dynamic_cast<Context2DData *>(content));
    auto contextData = static_cast<Context2DData *>(content);

    auto items = vtkSmartPointer<vtkPlotCollection>::New();

    vtkCollectionSimpleIterator it;
    contextData->plots()->InitTraversal(it);
    while (auto item = contextData->plots()->GetNextPlot(it))
    {
        items->AddItem(item);
        m_chart->AddPlot(item);
    }

    assert(!m_plots.contains(contextData));
    m_plots.insert(contextData, items);


    addConnectionForContent(content,
        connect(contextData, &Context2DData::plotCollectionChanged,
        [this, contextData] () { fetchContextItems(contextData); }));

    addConnectionForContent(content,
        connect(contextData, &Context2DData::visibilityChanged,
        [this, contextData] (bool) { dataVisibilityChanged(contextData); }));

    dataVisibilityChanged(contextData);
}

void RendererImplementationPlot::onRemoveContent(AbstractVisualizedData * content, unsigned int /*subViewIndex*/)
{
    assert(dynamic_cast<Context2DData *>(content));
    auto contextData = static_cast<Context2DData *>(content);

    if (contextData == m_selectedPlot)
    {
        m_renderView.clearSelection();
    }

    vtkSmartPointer<vtkPlotCollection> items = m_plots.take(contextData);
    assert(items);

    vtkCollectionSimpleIterator it;
    items->InitTraversal(it);
    while (auto item = items->GetNextPlot(it))
    {
        m_chart->RemovePlotInstance(item);
    }

    m_contextView->ResetCamera();
}

void RendererImplementationPlot::onSetSelection(const VisualizationSelection & selection)
{
    if (selection.indexType != IndexType::points)
    {
        return;
    }

    assert(selection.visualization);

    // Note: selection.visualization might not be a Context2DData *.
    // That's a safe cast only if the visualization is contained in m_plots (which needs to be
    // checked anyways).
    const auto & plots = m_plots.value(static_cast<Context2DData *>(selection.visualization));
    if (!plots)
    {
        return;
    }

    assert(plots->GetNumberOfItems() == 1);

    plots->InitTraversal();
    auto plot = plots->GetNextPlot();

    if (m_selectedPlot == selection.visualization)
    {
        if (auto previousIndices = plot->GetSelection())
        {
            if (previousIndices->GetNumberOfValues() == static_cast<vtkIdType>(selection.indices.size()))
            {
                bool changed = false;
                for (vtkIdType i = 0; i < previousIndices->GetNumberOfValues(); ++i)
                {
                    if (previousIndices->GetValue(i) != selection.indices[static_cast<size_t>(i)])
                    {
                        changed = true;
                        break;
                    }
                }
                if (!changed)
                {
                    return;
                }
            }
        }
    }

    m_selectedPlot = static_cast<Context2DData *>(selection.visualization);

    auto indices = plot->GetSelection()
        ? vtkSmartPointer<vtkIdTypeArray>(plot->GetSelection())
        : vtkSmartPointer<vtkIdTypeArray>::New();
    indices->Resize(selection.indices.size());
    for (size_t i = 0; i < selection.indices.size(); ++i)
    {
        indices->SetValue(static_cast<vtkIdType>(i), selection.indices[i]);
    }

    if (!plot->GetSelection())
    {
        plot->SetSelection(indices);
    }

    render();
}

void RendererImplementationPlot::onClearSelection()
{
    if (!m_selectedPlot)
    {
        return;
    }

    auto && plots = m_selectedPlot->plots();
    m_selectedPlot = nullptr;

    vtkCollectionSimpleIterator it;
    plots->InitTraversal(it);
    while (auto plot = plots->GetNextPlot(it))
    {
        if (auto indices = plot->GetSelection())
        {
            indices->SetNumberOfValues(0);
        }
    }
}

Context2DData * RendererImplementationPlot::contextDataContaining(const vtkPlot & plot) const
{
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
    {
        auto && plotList = it.value();

        for (plotList->InitTraversal(); auto p = plotList->GetNextPlot(); )
        {
            if (p == &plot)
            {
                return it.key();
            }
        }
    }

    return nullptr;
}

void RendererImplementationPlot::setAxesVisibility(bool /*visible*/)
{
}

bool RendererImplementationPlot::axesAutoUpdate() const
{
    return m_axesAutoUpdate;
}

void RendererImplementationPlot::setAxesAutoUpdate(bool enable)
{
    m_axesAutoUpdate = enable;
}

vtkChartXY * RendererImplementationPlot::chart()
{
    return m_chart;
}

vtkContextView * RendererImplementationPlot::contextView()
{
    return m_contextView;
}

std::unique_ptr<AbstractVisualizedData> RendererImplementationPlot::requestVisualization(DataObject & dataObject) const
{
    return dataObject.createContextData();
}

void RendererImplementationPlot::initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    m_contextView = vtkSmartPointer<vtkContextView>::New();
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_contextView->SetRenderWindow(m_renderWindow);

    m_chart = vtkSmartPointer<ChartXY>::New();
    m_chart->SetShowLegend(true);
    FontHelper::configureTextProperty(*m_chart->GetTitleProperties());
    FontHelper::configureTextProperty(*m_chart->GetLegend()->GetLabelProperties());
    for (const int axisId : {vtkAxis::LEFT, vtkAxis::BOTTOM, vtkAxis::RIGHT, vtkAxis::TOP})
    {
        auto axis = m_chart->GetAxis(axisId);
        FontHelper::configureTextProperty(*axis->GetTitleProperties());
        FontHelper::configureTextProperty(*axis->GetLabelProperties());
    }

    m_contextView->GetScene()->AddItem(m_chart);

    m_chart->AddObserver(ChartXY::PlotSelectedEvent, this, &RendererImplementationPlot::handlePlotSelectionEvent);

    m_isInitialized = true;
}

void RendererImplementationPlot::updateBounds()
{
    if (m_axesAutoUpdate)
    {
        m_chart->RecalculateBounds();
    }
}

void RendererImplementationPlot::fetchContextItems(Context2DData * data)
{
    vtkSmartPointer<vtkPlotCollection> items = m_plots.value(data);
    assert(items);

    // TODO add/remove changes only

    // remove all old props from the renderer
    vtkCollectionSimpleIterator it;
    items->InitTraversal(it);
    while (vtkPlot * item = items->GetNextPlot(it))
    {
        m_chart->RemovePlotInstance(item);
    }
    items->RemoveAllItems();

    // insert all new props

    data->plots()->InitTraversal(it);
    while (auto item = data->plots()->GetNextPlot(it))
    {
        items->AddItem(item);
        m_chart->AddItem(item);
    }

    render();
}

void RendererImplementationPlot::dataVisibilityChanged(Context2DData * data)
{
    if (data->isVisible())
    {
        connect(&data->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationPlot::updateBounds);
    }
    else
    {
        disconnect(&data->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationPlot::updateBounds);

        if (m_selectedPlot == data)
        {
            m_renderView.clearSelection();
        }
    }
}

void RendererImplementationPlot::handlePlotSelectionEvent(vtkObject * DEBUG_ONLY(subject), unsigned long /*eventId*/, void * callData)
{
    assert(subject == m_chart.Get());

    auto plot = reinterpret_cast<vtkPlot *>(callData);

    if (!plot)
    {
        m_renderView.clearSelection();
        return;
    }

    auto contextData = contextDataContaining(*plot);
    assert(contextData);

    auto selection = VisualizationSelection(
        contextData,
        0,
        IndexType::points);

    if (auto indexArray = plot->GetSelection())
    {
        selection.indices.resize(indexArray->GetNumberOfValues());
        for (vtkIdType i = 0; i < indexArray->GetNumberOfValues(); ++i)
        {
            selection.indices[static_cast<size_t>(i)] = indexArray->GetValue(i);
        }
    }

    m_selectedPlot = contextData;

    m_renderView.setVisualizationSelection(selection);
}
