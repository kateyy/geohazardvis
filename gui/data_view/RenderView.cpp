#include "RenderView.h"
#include "ui_RenderView.h"

#include <cassert>

#include <QMessageBox>

#include <vtkRenderWindow.h>

#include <core/types.h>
#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/scalar_mapping/ScalarToColorMapping.h>
#include <gui/data_view/RendererImplementationNull.h>
#include <gui/data_view/RendererImplementation3D.h>

#include <gui/SelectionHandler.h>


RenderView::RenderView(
    int index,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_ui(new Ui_RenderView())
    , m_implementation(nullptr)
    , m_scalarMapping(new ScalarToColorMapping())
    , m_axesEnabled(true)
{
    m_ui->setupUi(this);

    setupRenderer();

    updateTitle();

    SelectionHandler::instance().addRenderView(this);
}

RenderView::~RenderView()
{
    SelectionHandler::instance().removeRenderView(this); 

    for (RenderedData * rendered : m_renderedData)
        emit beforeDeleteRenderedData(rendered);

    for (RenderedData * rendered : m_renderedDataCache)
        emit beforeDeleteRenderedData(rendered);

    qDeleteAll(m_renderedData);
    qDeleteAll(m_renderedDataCache);

    delete m_implementation;
    delete m_scalarMapping;
}

bool RenderView::isTable() const
{
    return false;
}

bool RenderView::isRenderer() const
{
    return true;
}

QString RenderView::friendlyName() const
{
    QString name;
    for (RenderedData * renderedData : m_renderedData)
        name += ", " + renderedData->dataObject()->name();

    if (name.isEmpty())
        name = "(empty)";
    else
        name.remove(0, 2);

    name = QString::number(index()) + ": " + name;

    return name;
}

ContentType RenderView::contentType() const
{
    return implementation().contentType();
}

void RenderView::render()
{
    implementation().render();
}

QWidget * RenderView::contentWidget()
{
    return m_ui->qvtkMain;
}

void RenderView::highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId)
{
    implementation().highlightData(dataObject, itemId);
}

void RenderView::setupRenderer()
{
    m_implementation = new RendererImplementation3D(*this);
    m_implementation->apply(m_ui->qvtkMain);

    connect(m_implementation, &RendererImplementation::dataSelectionChanged, this, &RenderView::updateGuiForSelectedData);
}

void RenderView::ShowInfo(const QStringList & info)
{
    setToolTip(info.join('\n'));
}

RenderedData * RenderView::addDataObject(DataObject * dataObject)
{
    updateTitle(dataObject->name() + " (loading to GPU)");
    QApplication::processEvents();

    assert(dataObject->is3D() == contains3dData());

    RenderedData * renderedData = dataObject->createRendered();
    if (!renderedData)
        return nullptr;

    implementation().addRenderedData(renderedData);

    m_renderedData << renderedData;

    connect(renderedData, &RenderedData::geometryChanged, this, &RenderView::render);

    m_dataObjectToRendered.insert(dataObject, renderedData);

    return renderedData;
}

void RenderView::addDataObjects(const QList<DataObject *> & uncheckedDataObjects, QList<DataObject *> & incompatibleObjects)
{
    if (uncheckedDataObjects.isEmpty())
        return;

    bool wasEmpty = m_renderedData.isEmpty();

    if (wasEmpty)
        emit resetImplementation(uncheckedDataObjects);

    RenderedData * aNewObject = nullptr;

    QList<DataObject *> dataObjects = implementation().filterCompatibleObjects(uncheckedDataObjects, incompatibleObjects);

    if (dataObjects.isEmpty())
        return;

    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * cachedRendered = m_dataObjectToRendered.value(dataObject);

        // create new rendered representation
        if (!cachedRendered)
        {
            aNewObject = addDataObject(dataObject);
            continue;
        }

        aNewObject = cachedRendered;

        // reuse cached data
        if (m_renderedData.contains(cachedRendered))
        {
            assert(false);
            continue;
        }

        assert(m_renderedDataCache.count(cachedRendered) == 1);
        m_renderedDataCache.removeOne(cachedRendered);
        m_renderedData << cachedRendered;
        cachedRendered->setVisible(true);
    }

    if (aNewObject)
    {
        updateGuiForSelectedData(aNewObject);

        emit renderedDataChanged();
    }

    if (aNewObject)
    {
        implementation().resetCamera(wasEmpty);
    }
}

void RenderView::hideDataObjects(const QList<DataObject *> & dataObjects)
{
    bool changed = false;
    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * rendered = m_dataObjectToRendered.value(dataObject);
        if (!rendered)
            continue;

        // cached data is only accessible internally in the view, so let others know that it's gone for the moment
        emit beforeDeleteRenderedData(rendered);

        // move data to cache if it isn't already invisible
        if (m_renderedData.removeOne(rendered))
        {
            rendered->setVisible(false);
            m_renderedDataCache << rendered;

            changed = true;
        }
        assert(!m_renderedData.contains(rendered));
        assert(m_renderedDataCache.count(rendered) == 1);
    }

    if (!changed)
        return;

    updateGuiForRemovedData();

    emit renderedDataChanged();

    render();
}

bool RenderView::contains(DataObject * dataObject) const
{
    RenderedData * renderedData = m_dataObjectToRendered.value(dataObject, nullptr);
    if (!renderedData)
        return false;
    return renderedData->isVisible();
}

void RenderView::removeDataObject(DataObject * dataObject)
{
    RenderedData * renderedData = m_dataObjectToRendered.value(dataObject, nullptr);

    // we didn't render this object
    if (!renderedData)
        return;

    implementation().removeRenderedData(renderedData);

    emit beforeDeleteRenderedData(renderedData);

    QList<RenderedData *> toDelete = removeFromInternalLists({ dataObject });

    updateGuiForRemovedData();

    emit renderedDataChanged();

    render();

    qDeleteAll(toDelete);
}

void RenderView::removeDataObjects(const QList<DataObject *> & dataObjects)
{
    for (DataObject * dataObject : dataObjects)
        removeDataObject(dataObject);
}

QList<RenderedData *> RenderView::removeFromInternalLists(QList<DataObject *> dataObjects)
{
    QList<RenderedData *> toDelete;
    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * rendered = m_dataObjectToRendered.value(dataObject, nullptr);
        assert(rendered);

        m_dataObjectToRendered.remove(dataObject);
        assert(m_renderedData.count(rendered) + m_renderedDataCache.count(rendered) == 1);
        if (!m_renderedData.removeOne(rendered))
            m_renderedDataCache.removeOne(rendered);

        toDelete << rendered;
    }

    return toDelete;
}

QList<DataObject *> RenderView::dataObjects() const
{
    QList<DataObject *> objs;
    for (RenderedData * r : m_renderedData)
        objs << r->dataObject();

    return objs;
}

const QList<RenderedData *> & RenderView::renderedData() const
{
    return m_renderedData;
}

DataObject * RenderView::highlightedData() const
{
    DataObject * highlighted = implementation().highlightedData();

    if (!highlighted && !m_renderedData.isEmpty())
        highlighted = m_renderedData.first()->dataObject();

    return highlighted;
}

RenderedData * RenderView::highlightedRenderedData() const
{
    return m_dataObjectToRendered.value(highlightedData());
}

void RenderView::lookAtData(DataObject * dataObject, vtkIdType itemId)
{
    implementation().lookAtData(dataObject, itemId);
}

ScalarToColorMapping * RenderView::scalarMapping()
{
    return m_scalarMapping;
}

RendererImplementation & RenderView::implementation() const
{
    //static RendererImplementationNull nullImpl;

    /*if (!m_implementation)
        return nullImpl;*/

    assert(m_implementation);

    return *m_implementation;
}

vtkRenderWindow * RenderView::renderWindow()
{
    return m_ui->qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * RenderView::renderWindow() const
{
    return m_ui->qvtkMain->GetRenderWindow();
}

void RenderView::setEnableAxes(bool enabled)
{
    if (m_axesEnabled == enabled)
        return;

    m_axesEnabled = enabled;

    implementation().setAxesVisibility(m_axesEnabled && !m_renderedData.isEmpty());
}

bool RenderView::axesEnabled() const
{
    return m_axesEnabled;
}

bool RenderView::contains3dData() const
{
    return implementation().contentType() == ContentType::Rendered3D;
}

void RenderView::updateGuiForContent()
{
    RenderedData * focus = m_dataObjectToRendered.value(implementation().highlightedData());
    if (!focus)
        focus = m_renderedData.value(0, nullptr);

    updateGuiForSelectedData(focus);
}

void RenderView::updateGuiForSelectedData(RenderedData * renderedData)
{
    m_scalarMapping->setRenderedData(m_renderedData);

    DataObject * current = renderedData ? renderedData->dataObject() : nullptr;

    updateTitle();

    emit selectedDataChanged(this, current);
}

void RenderView::updateGuiForRemovedData()
{
    RenderedData * nextSelection = m_renderedData.isEmpty()
        ? nullptr : m_renderedData.first();
    
    updateGuiForSelectedData(nextSelection);
}
