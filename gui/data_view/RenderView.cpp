#include "RenderView.h"

#include <cassert>

#include <QApplication>
#include <QMessageBox>

#include <vtkRenderWindow.h>

#include <core/types.h>
#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/macros.h>
#include <core/utility/memory.h>

#include <gui/data_view/RendererImplementation.h>
#include <gui/data_view/RendererImplementationSwitch.h>


RenderView::RenderView(
    DataMapping & dataMapping,
    int index,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(dataMapping, index, parent, flags)
    , m_implementationSwitch(std::make_unique<RendererImplementationSwitch>(*this))
    , m_closingRequested(false)
{
    updateTitle();

    updateImplementation({});
}

RenderView::~RenderView()
{
    for (auto & rendered : m_contents)
        emit beforeDeleteVisualization(rendered.get());

    for (auto & rendered : m_contentCache)
        emit beforeDeleteVisualization(rendered.get());

    m_contents.clear();
    m_contentCache.clear();
}

QString RenderView::friendlyName() const
{
    QString name;
    for (auto & renderedData : m_contents)
        name += ", " + renderedData->dataObject().name();

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

void RenderView::closeEvent(QCloseEvent * event)
{
    m_closingRequested = true;

    AbstractRenderView::closeEvent(event);
}

void RenderView::axesEnabledChangedEvent(bool enabled)
{
    implementation().setAxesVisibility(enabled && !m_contents.empty());
}

void RenderView::updateImplementation(const QList<DataObject *> & contents)
{
    implementation().deactivate(qvtkWidget());

    disconnect(&implementation(), &RendererImplementation::dataSelectionChanged,
        this, &RenderView::updateGuiForSelectedData);

    assert(m_contents.empty());
    m_contentCache.clear();
    m_dataObjectToVisualization.clear();

    m_implementationSwitch->findSuitableImplementation(contents);

    implementation().activate(qvtkWidget());

    connect(&implementation(), &RendererImplementation::dataSelectionChanged,
        this, &RenderView::updateGuiForSelectedData);

    emit implementationChanged();
}

AbstractVisualizedData * RenderView::addDataObject(DataObject * dataObject)
{
    updateTitle(dataObject->name() + " (loading to GPU)");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // abort if we received a close event in QApplication::processEvents 
    if (m_closingRequested)
        return nullptr;

    if (m_deletedData.remove(dataObject))
        return nullptr;

    auto newContent = implementation().requestVisualization(*dataObject);

    if (!newContent)
        return nullptr;

    implementation().addContent(newContent.get(), 0);

    auto * newContentPtr = newContent.get();
    m_contents.push_back(std::move(newContent));

    connect(newContentPtr, &AbstractVisualizedData::geometryChanged, this, &RenderView::render);

    m_dataObjectToVisualization.insert(dataObject, newContentPtr);

    return newContentPtr;
}

void RenderView::showDataObjectsImpl(const QList<DataObject *> & uncheckedDataObjects, QList<DataObject *> & incompatibleObjects, unsigned int /*subViewIndex*/)
{
    if (uncheckedDataObjects.isEmpty())
    {
        return;
    }

    bool wasEmpty = m_contents.empty();

    if (wasEmpty)
    {
        updateImplementation(uncheckedDataObjects);
    }

    AbstractVisualizedData * aNewObject = nullptr;

    QList<DataObject *> dataObjects = implementation().filterCompatibleObjects(uncheckedDataObjects, incompatibleObjects);

    if (dataObjects.isEmpty())
    {
        return;
    }

    for (DataObject * dataObject : dataObjects)
    {
        AbstractVisualizedData * previouslyRendered = m_dataObjectToVisualization.value(dataObject);

        // create new rendered representation
        if (!previouslyRendered)
        {
            aNewObject = addDataObject(dataObject);

            if (m_closingRequested) // just abort here, if we processed a close event while addDataObject()
                return;

            continue;
        }

        // reuse currently rendered / cached data

        auto contensIt = findUnique(m_contents, previouslyRendered);
        if (contensIt != m_contents.end())
        {
            continue;
        }

        aNewObject = previouslyRendered;

        auto cacheIt = findUnique(m_contentCache, previouslyRendered);
        m_contents.push_back(std::move(*cacheIt));
        m_contentCache.erase(cacheIt);

        previouslyRendered->setVisible(true);
    }

    if (aNewObject)
    {
        implementation().renderViewContentsChanged();

        updateGuiForSelectedData(aNewObject);

        emit visualizationsChanged();
    }

    if (aNewObject)
    {
        implementation().resetCamera(wasEmpty, 0);
    }

    updateTitle();
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
        auto contentsIt = findUnique(m_contents, rendered);
        if (contentsIt != m_contents.end())
        {
            m_contentCache.push_back(std::move(*contentsIt));
            m_contents.erase(contentsIt);

            rendered->setVisible(false);

            changed = true;
        }
        assert(!containsUnique(m_contents, (AbstractVisualizedData*)nullptr));
    }

    if (!changed)
        return;

    updateGuiForRemovedData();

    implementation().renderViewContentsChanged();

    emit visualizationsChanged();

    render();
}

QList<DataObject *> RenderView::dataObjectsImpl(int /*subViewIndex*/) const
{
    QList<DataObject *> dataObjects;
    for (auto && vis : m_contents)
        dataObjects << &vis->dataObject();

    return dataObjects;
}

void RenderView::removeDataObject(DataObject * dataObject)
{
    AbstractVisualizedData * renderedData = m_dataObjectToVisualization.value(dataObject, nullptr);

    // for the case that we are currently loading this object
    m_deletedData << dataObject;

    // we didn't render this object
    if (!renderedData)
        return;

    implementation().removeContent(renderedData, 0);

    emit beforeDeleteVisualization(renderedData);

    auto toDelete = removeFromInternalLists({ dataObject });

    updateGuiForRemovedData();

    implementation().renderViewContentsChanged();

    emit visualizationsChanged();

    render();

    toDelete.clear();

    m_deletedData.clear();
}

void RenderView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    for (DataObject * dataObject : dataObjects)
        removeDataObject(dataObject);
}

std::vector<std::unique_ptr<AbstractVisualizedData>> RenderView::removeFromInternalLists(QList<DataObject *> dataObjects)
{
    std::vector<std::unique_ptr<AbstractVisualizedData>> toDelete;
    for (DataObject * dataObject : dataObjects)
    {
        AbstractVisualizedData * rendered = m_dataObjectToVisualization.value(dataObject, nullptr);
        assert(rendered);

        m_dataObjectToVisualization.remove(dataObject);

        auto contentsIt = findUnique(m_contents, rendered);
        if (contentsIt != m_contents.end())
        {
            toDelete.push_back(std::move(*contentsIt));
            m_contents.erase(contentsIt);
        }
        else
        {
            auto cacheIt = findUnique(m_contentCache, rendered);
            if (cacheIt != m_contentCache.end())
            {
                toDelete.push_back(std::move(*cacheIt));
                m_contentCache.erase(cacheIt);
            }
        }
    }

    return toDelete;
}

QList<AbstractVisualizedData *> RenderView::visualizationsImpl(int /*subViewIndex*/) const
{
    QList<AbstractVisualizedData *> vis;
    for (auto & content : m_contents)
        vis << content.get();

    return vis;
}

DataObject * RenderView::selectedData() const
{
    if (auto vis = selectedDataVisualization())
    {
        return &vis->dataObject();
    }

    return nullptr;
}

AbstractVisualizedData * RenderView::selectedDataVisualization() const
{
    return implementation().selectedData();
}

void RenderView::lookAtData(DataObject & dataObject, vtkIdType index, IndexType indexType, int subViewIndex)
{
    auto vis = m_dataObjectToVisualization.value(&dataObject, nullptr);

    if (!vis)
        return;

    lookAtData(*vis, index, indexType, subViewIndex);
}

void RenderView::lookAtData(AbstractVisualizedData & vis, vtkIdType index, IndexType indexType, int DEBUG_ONLY(subViewIndex))
{
    assert(subViewIndex == 0 || subViewIndex == -1);
    assert(containsUnique(m_contents, &vis));

    implementation().lookAtData(vis, index, indexType, 0u);
}

AbstractVisualizedData * RenderView::visualizationFor(DataObject * dataObject, int subViewIndex) const
{
    if (subViewIndex != -1 && subViewIndex != 0)
        return nullptr;

    return m_dataObjectToVisualization.value(dataObject, nullptr);
}

int RenderView::subViewContaining(const AbstractVisualizedData & visualizedData) const
{
    // single view implementation: if we currently show it, it's in the view 0

    if (containsUnique(m_contents, &visualizedData))
        return 0;

    return -1;
}

RendererImplementation & RenderView::implementation() const
{
    return m_implementationSwitch->currentImplementation();
}

void RenderView::updateGuiForSelectedData(AbstractVisualizedData * renderedData)
{
    DataObject * current = renderedData ? &renderedData->dataObject() : nullptr;

    updateTitle();

    emit selectedDataChanged(this, current);
}

void RenderView::updateGuiForRemovedData()
{
    AbstractVisualizedData * nextSelection = m_contents.empty()
        ? nullptr : m_contents.front().get();
    
    updateGuiForSelectedData(nextSelection);
}
