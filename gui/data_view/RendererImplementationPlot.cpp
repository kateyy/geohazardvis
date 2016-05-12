#include "RendererImplementationPlot.h"

#include <cassert>

#include <vtkAxis.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkIdTypeArray.h>
#include <vtkPlot.h>
#include <vtkRendererCollection.h>

#include <core/t_QVTKWidget.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/context2D_data/Context2DData.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <core/utility/font.h>
#include <core/utility/macros.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/ChartXY.h>


bool RendererImplementationPlot::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementationPlot>();


RendererImplementationPlot::RendererImplementationPlot(AbstractRenderView & renderView)
    : RendererImplementation(renderView)
    , m_isInitialized{ false }
    , m_axesAutoUpdate{ true }
{
    assert(renderView.numberOfSubViews() == 1); // multi view not implemented yet. Is that even possible with the vtkContextView?
}

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
    for (DataObject * dataObject : dataObjects)
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

    for (DataObject * dataObject : dataObjects)
        if (dataObject->dataTypeName() == "image profile") // hard-coded for now
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;

    return compatible;
}

void RendererImplementationPlot::activate(t_QVTKWidget & qvtkWidget)
{
    initialize();

    // see also: vtkRenderViewBase documentation
    m_contextView->SetInteractor(qvtkWidget.GetInteractor());
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
        return;

    m_contextView->Render();
}

vtkRenderWindowInteractor * RendererImplementationPlot::interactor()
{
    return m_contextView->GetInteractor();
}

void RendererImplementationPlot::setSelectedData(AbstractVisualizedData * vis, vtkIdType index, IndexType indexType)
{
    auto indices = vtkSmartPointer<vtkIdTypeArray>::New();
    indices->SetNumberOfValues(1);
    indices->SetValue(1, index);

    setSelectedData(vis, index, indexType);
}

void RendererImplementationPlot::setSelectedData(AbstractVisualizedData * vis, vtkIdTypeArray & indices, IndexType indexType)
{
    Context2DData * contextData = nullptr;
    if (indexType != IndexType::points
        || (nullptr == (contextData = dynamic_cast<Context2DData *>(vis))))
    {
        clearSelection();
        return;
    }

    const auto & plots = m_plots.value(contextData, nullptr);
    if (!plots)
    {
        clearSelection();
        return;
    }

    assert(plots->GetNumberOfItems() == 1);

    plots->InitTraversal();
    auto plot = plots->GetNextPlot();

    plot->SetSelection(&indices);
}

void RendererImplementationPlot::clearSelection()
{
    for (vtkIdType i = 0; i < m_chart->GetNumberOfPlots(); ++i)
    {
        m_chart->GetPlot(i)->SetSelection(nullptr);
    }
}

AbstractVisualizedData * RendererImplementationPlot::selectedData() const
{
    auto selectedPlot = m_chart->selectedPlot();

    if (!selectedPlot)
    {
        return nullptr;
    }

    return contextDataContaining(*selectedPlot);
}

vtkIdType RendererImplementationPlot::selectedIndex() const
{
    auto selectedPlot = m_chart->selectedPlot();

    if (!selectedPlot)
    {
        return -1;
    }

    auto selection = m_chart->selectedPlot()->GetSelection();

    if (!selection || selection->GetSize() == 0)
    {
        return -1;
    }

    return selection->GetValue(0);
}

IndexType RendererImplementationPlot::selectedIndexType() const
{
    return IndexType::points;
}

void RendererImplementationPlot::lookAtData(AbstractVisualizedData & /*vis*/, vtkIdType /*index*/, IndexType /*indexType*/, unsigned int /*subViewIndex*/)
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
    Context2DData * contextData = static_cast<Context2DData *>(content);

    auto items = vtkSmartPointer<vtkPlotCollection>::New();

    vtkCollectionSimpleIterator it;
    contextData->plots()->InitTraversal(it);
    while (vtkPlot * item = contextData->plots()->GetNextPlot(it))
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
    Context2DData * contextData = static_cast<Context2DData *>(content);

    vtkSmartPointer<vtkPlotCollection> items = m_plots.take(contextData);
    assert(items);

    vtkCollectionSimpleIterator it;
    items->InitTraversal(it);
    while (vtkPlot * item = items->GetNextPlot(it))
    {
        m_chart->RemovePlotInstance(item);
    }

    m_contextView->ResetCamera();
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
        return;

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
        m_chart->RecalculateBounds();
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
        m_chart->RemovePlotInstance(item);
    items->RemoveAllItems();

    // insert all new props

    data->plots()->InitTraversal(it);
    while (vtkPlot * item = data->plots()->GetNextPlot(it))
    {
        items->AddItem(item);
        m_chart->AddItem(item);
    }

    render();
}

void RendererImplementationPlot::dataVisibilityChanged(Context2DData * data)
{
    if (data->isVisible())
        connect(&data->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationPlot::updateBounds);
    else
        disconnect(&data->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationPlot::updateBounds);
}

void RendererImplementationPlot::handlePlotSelectionEvent(vtkObject * DEBUG_ONLY(subject), unsigned long /*eventId*/, void * callData)
{
    assert(subject == m_chart.Get());

    auto plot = reinterpret_cast<vtkPlot *>(callData);

    if (!plot)
    {
        emit dataSelectionChanged(nullptr);
        return;
    }

    auto vis = contextDataContaining(*plot);

    emit dataSelectionChanged(vis);

    m_renderView.objectPicked(&vis->dataObject(), 
        selectedIndex(), 
        selectedIndexType());
}
