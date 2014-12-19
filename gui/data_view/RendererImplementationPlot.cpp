#include "RendererImplementationPlot.h"

#include <cassert>

#include <QVTKWidget.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkPlot.h>
#include <vtkRenderWindow.h>

#include <core/types.h>
#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/context2D_data/Context2DData.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <gui/data_view/RenderView.h>


bool RendererImplementationPlot::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementationPlot>();


RendererImplementationPlot::RendererImplementationPlot(RenderView & renderView, QObject * parent)
    : RendererImplementation(renderView, parent)
    , m_isInitialized(false)
{
}

RendererImplementationPlot::~RendererImplementationPlot()
{
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
        if (dataObject->dataTypeName() == "image profile") // hardcoded for now
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;

    return compatible;
}

void RendererImplementationPlot::activate(QVTKWidget * qvtkWidget)
{
    initialize();

    m_contextView->SetInteractor(qvtkWidget->GetInteractor());
    qvtkWidget->SetRenderWindow(m_contextView->GetRenderWindow());
}

void RendererImplementationPlot::render()
{
    m_contextView->Render();
}

vtkRenderWindowInteractor * RendererImplementationPlot::interactor()
{
    return m_contextView->GetInteractor();
}

void RendererImplementationPlot::addContent(AbstractVisualizedData * content)
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

    connect(contextData, &Context2DData::plotCollectionChanged,
        [this, contextData] () { fetchContextItems(contextData); });

    connect(contextData, &Context2DData::visibilityChanged,
        [this, contextData] (bool) { dataVisibilityChanged(contextData); });

    dataVisibilityChanged(contextData);
}

void RendererImplementationPlot::removeContent(AbstractVisualizedData * content)
{
    assert(dynamic_cast<Context2DData *>(content));
    Context2DData * contextData = static_cast<Context2DData *>(content);

    vtkSmartPointer<vtkContextItemCollection> items = m_plots.take(contextData);
    assert(items);

    vtkCollectionSimpleIterator it;
    items->InitTraversal(it);
    while (vtkAbstractContextItem * item = items->GetNextContextItem(it))
        m_chart->RemoveItem(item);

    m_contextView->ResetCamera();
}

void RendererImplementationPlot::highlightData(DataObject * /*dataObject*/, vtkIdType /*itemId*/)
{
}

DataObject * RendererImplementationPlot::highlightedData()
{
    return nullptr;
}

void RendererImplementationPlot::lookAtData(DataObject * /*dataObject*/, vtkIdType /*itemId*/)
{;
}

void RendererImplementationPlot::resetCamera(bool /*toInitialPosition*/)
{
    m_contextView->ResetCamera();

    render();
}

void RendererImplementationPlot::setAxesVisibility(bool /*visible*/)
{
    //m_axesActor->SetVisibility(visible);

    //render();
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
    m_contextView->GetScene()->AddItem(m_chart);

    m_isInitialized = true;
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
        m_chart->RemoveItem(item);
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

void RendererImplementationPlot::dataVisibilityChanged(Context2DData * /*data*/)
{
}
