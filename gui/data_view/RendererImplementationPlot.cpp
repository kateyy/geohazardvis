#include "RendererImplementationPlot.h"

#include <cassert>

#include <QVTKWidget.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkPlot.h>
#include <vtkRenderWindow.h>

#include <core/types.h>
#include <core/utility/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/context2D_data/Context2DData.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <gui/data_view/AbstractRenderView.h>


bool RendererImplementationPlot::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementationPlot>();


RendererImplementationPlot::RendererImplementationPlot(AbstractRenderView & renderView, QObject * parent)
    : RendererImplementation(renderView, parent)
    , m_isInitialized(false)
    , m_axesAutoUpdate(true)
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
        if (Context2DData * cd = dataObject->createContextData())
        {
            delete cd;
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

void RendererImplementationPlot::activate(QVTKWidget * qvtkWidget)
{
    initialize();

    // see also: vtkRenderViewBase documentation
    m_contextView->SetInteractor(qvtkWidget->GetInteractor());
    qvtkWidget->SetRenderWindow(m_contextView->GetRenderWindow());
}

void RendererImplementationPlot::deactivate(QVTKWidget * qvtkWidget)
{
    // the render window belongs to the context view
    qvtkWidget->SetRenderWindow(nullptr);
    // the interactor is provided by the qvtkWidget
    m_contextView->SetInteractor(nullptr);
}

void RendererImplementationPlot::render()
{
    m_contextView->Render();
}

vtkRenderWindowInteractor * RendererImplementationPlot::interactor()
{
    return m_contextView->GetInteractor();
}

void RendererImplementationPlot::onAddContent(AbstractVisualizedData * content, unsigned int /*subViewIndex*/)
{
    assert(dynamic_cast<Context2DData *>(content));
    Context2DData * contextData = static_cast<Context2DData *>(content);

    VTK_CREATE(vtkPlotCollection, items);

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

void RendererImplementationPlot::setSelectedData(DataObject * /*dataObject*/, vtkIdType /*itemId*/)
{
}

DataObject * RendererImplementationPlot::selectedData() const
{
    return nullptr;
}

vtkIdType RendererImplementationPlot::selectedIndex() const
{
    return -1;
}

void RendererImplementationPlot::lookAtData(DataObject * /*dataObject*/, vtkIdType /*itemId*/)
{
}

void RendererImplementationPlot::resetCamera(bool /*toInitialPosition*/)
{
    m_contextView->ResetCamera();

    render();
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

AbstractVisualizedData * RendererImplementationPlot::requestVisualization(DataObject * dataObject) const
{
    return dataObject->createContextData();
}

void RendererImplementationPlot::initialize()
{
    if (m_isInitialized)
        return;

    m_contextView = vtkSmartPointer<vtkContextView>::New();
    m_chart = vtkSmartPointer<vtkChartXY>::New();
    m_chart->SetShowLegend(true);
    m_contextView->GetScene()->AddItem(m_chart);

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
        connect(data->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationPlot::updateBounds);
    else
        disconnect(data->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationPlot::updateBounds);
}
