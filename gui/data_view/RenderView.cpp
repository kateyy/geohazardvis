#include "RenderView.h"
#include "ui_RenderView.h"

#include <cassert>

#include <QMessageBox>

#include <vtkRenderWindow.h>

#include <core/types.h>
#include <core/utility/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/AbstractVisualizedData.h>

#include <gui/data_view/RendererImplementation.h>
#include <gui/data_view/RendererImplementationSwitch.h>
#include <gui/SelectionHandler.h>


RenderView::RenderView(
    int index,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(index, parent, flags)
    , m_ui(new Ui_RenderView())
    , m_implementationSwitch(new RendererImplementationSwitch(*this))
{
    m_ui->setupUi(this);

    updateTitle();

    SelectionHandler::instance().addRenderView(this);
}

RenderView::~RenderView()
{
    SelectionHandler::instance().removeRenderView(this); 

    for (AbstractVisualizedData * rendered : m_contents)
        emit beforeDeleteVisualization(rendered);

    for (AbstractVisualizedData * rendered : m_contentCache)
        emit beforeDeleteVisualization(rendered);

    qDeleteAll(m_contents);
    qDeleteAll(m_contentCache);

    delete m_implementationSwitch;

    delete m_ui;
}

QString RenderView::friendlyName() const
{
    QString name;
    for (AbstractVisualizedData * renderedData : m_contents)
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
    implementation().setSelectedData(dataObject, itemId);
}

void RenderView::axesEnabledChangedEvent(bool enabled)
{
    implementation().setAxesVisibility(enabled && !m_contents.isEmpty());
}

void RenderView::updateImplementation(const QList<DataObject *> & contents)
{
    implementation().deactivate(m_ui->qvtkMain);

    disconnect(&implementation(), &RendererImplementation::dataSelectionChanged,
        this, &RenderView::updateGuiForSelectedData);

    assert(m_contents.isEmpty());
    m_contentCache.clear();
    m_dataObjectToVisualization.clear();

    m_implementationSwitch->findSuitableImplementation(contents);

    implementation().activate(m_ui->qvtkMain);

    connect(&implementation(), &RendererImplementation::dataSelectionChanged,
        this, &RenderView::updateGuiForSelectedData);
}

AbstractVisualizedData * RenderView::addDataObject(DataObject * dataObject)
{
    updateTitle(dataObject->name() + " (loading to GPU)");
    QApplication::processEvents();

    assert(dataObject->is3D() == (contentType() == ContentType::Rendered3D));

    AbstractVisualizedData * newContent = implementation().requestVisualization(dataObject);

    if (!newContent)
        return nullptr;

    implementation().addContent(newContent);

    m_contents << newContent;

    connect(newContent, &AbstractVisualizedData::geometryChanged, this, &RenderView::render);

    m_dataObjectToVisualization.insert(dataObject, newContent);

    return newContent;
}

void RenderView::addDataObjectsImpl(const QList<DataObject *> & uncheckedDataObjects, QList<DataObject *> & incompatibleObjects, unsigned int /*suViewIndex*/)
{
    if (uncheckedDataObjects.isEmpty())
        return;

    bool wasEmpty = m_contents.isEmpty();

    if (wasEmpty)
    {
        updateImplementation(uncheckedDataObjects);
    }

    AbstractVisualizedData * aNewObject = nullptr;

    QList<DataObject *> dataObjects = implementation().filterCompatibleObjects(uncheckedDataObjects, incompatibleObjects);

    if (dataObjects.isEmpty())
        return;

    for (DataObject * dataObject : dataObjects)
    {
        AbstractVisualizedData * cachedRendered = m_dataObjectToVisualization.value(dataObject);

        // create new rendered representation
        if (!cachedRendered)
        {
            aNewObject = addDataObject(dataObject);
            continue;
        }

        aNewObject = cachedRendered;

        // reuse cached data
        if (m_contents.contains(cachedRendered))
        {
            assert(false);
            continue;
        }

        assert(m_contentCache.count(cachedRendered) == 1);
        m_contentCache.removeOne(cachedRendered);
        m_contents << cachedRendered;
        cachedRendered->setVisible(true);
    }

    if (aNewObject)
    {
        updateGuiForSelectedData(aNewObject);

        emit visualizationsChanged();
    }

    if (aNewObject)
    {
        implementation().resetCamera(wasEmpty);
    }
}

void RenderView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int /*suViewIndex*/)
{
    bool changed = false;
    for (DataObject * dataObject : dataObjects)
    {
        AbstractVisualizedData * rendered = m_dataObjectToVisualization.value(dataObject);
        if (!rendered)
            continue;

        // cached data is only accessible internally in the view, so let others know that it's gone for the moment
        emit beforeDeleteVisualization(rendered);

        // move data to cache if it isn't already invisible
        if (m_contents.removeOne(rendered))
        {
            rendered->setVisible(false);
            m_contentCache << rendered;

            changed = true;
        }
        assert(!m_contents.contains(rendered));
        assert(m_contentCache.count(rendered) == 1);
    }

    if (!changed)
        return;

    updateGuiForRemovedData();

    emit visualizationsChanged();

    render();
}

void RenderView::removeDataObject(DataObject * dataObject)
{
    AbstractVisualizedData * renderedData = m_dataObjectToVisualization.value(dataObject, nullptr);

    // we didn't render this object
    if (!renderedData)
        return;

    implementation().removeContent(renderedData);

    emit beforeDeleteVisualization(renderedData);

    QList<AbstractVisualizedData *> toDelete = removeFromInternalLists({ dataObject });

    updateGuiForRemovedData();

    emit visualizationsChanged();

    render();

    qDeleteAll(toDelete);
}

void RenderView::removeDataObjectsImpl(const QList<DataObject *> & dataObjects)
{
    for (DataObject * dataObject : dataObjects)
        removeDataObject(dataObject);
}

QList<AbstractVisualizedData *> RenderView::removeFromInternalLists(QList<DataObject *> dataObjects)
{
    QList<AbstractVisualizedData *> toDelete;
    for (DataObject * dataObject : dataObjects)
    {
        AbstractVisualizedData * rendered = m_dataObjectToVisualization.value(dataObject, nullptr);
        assert(rendered);

        m_dataObjectToVisualization.remove(dataObject);
        assert(m_contents.count(rendered) + m_contentCache.count(rendered) == 1);
        if (!m_contents.removeOne(rendered))
            m_contentCache.removeOne(rendered);

        toDelete << rendered;
    }

    return toDelete;
}

QList<AbstractVisualizedData *> RenderView::visualizationsImpl(int /*subViewIndex*/) const
{
    return m_contents;
}

DataObject * RenderView::selectedData() const
{
    auto selected = implementation().selectedData();

    if (!selected && !m_contents.isEmpty())
        selected = m_contents.first()->dataObject();

    return selected;
}

AbstractVisualizedData * RenderView::selectedDataVisualization() const
{
    return m_dataObjectToVisualization.value(selectedData());
}

void RenderView::lookAtData(DataObject * dataObject, vtkIdType itemId)
{
    implementation().lookAtData(dataObject, itemId);
}

AbstractVisualizedData * RenderView::visualizationFor(DataObject * dataObject, int subViewIndex) const
{
    if (subViewIndex != -1 && subViewIndex != 0)
        return nullptr;

    return m_dataObjectToVisualization.value(dataObject, nullptr);
}

RendererImplementation & RenderView::implementation() const
{
    return m_implementationSwitch->currentImplementation();
}

vtkRenderWindow * RenderView::renderWindow()
{
    return m_ui->qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * RenderView::renderWindow() const
{
    return m_ui->qvtkMain->GetRenderWindow();
}

void RenderView::updateGuiForContent()
{
    AbstractVisualizedData * focus = m_dataObjectToVisualization.value(implementation().selectedData());
    if (!focus)
        focus = m_contents.value(0, nullptr);

    updateGuiForSelectedData(focus);
}

void RenderView::updateGuiForSelectedData(AbstractVisualizedData * renderedData)
{
    DataObject * current = renderedData ? renderedData->dataObject() : nullptr;

    updateTitle();

    emit selectedDataChanged(this, current);
}

void RenderView::updateGuiForRemovedData()
{
    AbstractVisualizedData * nextSelection = m_contents.isEmpty()
        ? nullptr : m_contents.first();
    
    updateGuiForSelectedData(nextSelection);
}
