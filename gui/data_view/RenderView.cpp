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
    , m_implementationSwitch{ std::make_unique<RendererImplementationSwitch>(*this) }
    , m_closingRequested{ false }
{
    updateTitle();
}

RenderView::~RenderView()
{
    signalClosing();

    QList<AbstractVisualizedData *> toDelete;

    for (auto & rendered : m_contents)
    {
        toDelete << rendered.get();
    }

    for (auto & rendered : m_contentCache)
    {
        toDelete << rendered.get();
    }

    emit beforeDeleteVisualizations(toDelete);
}

ContentType RenderView::contentType() const
{
    return implementation().contentType();
}

void RenderView::closeEvent(QCloseEvent * event)
{
    m_closingRequested = true;

    // remove all visualization and reset to null implementation to correctly cleanup OpenGL states
    prepareDeleteData(dataObjects());
    updateImplementation({});

    AbstractRenderView::closeEvent(event);
}

void RenderView::initializeRenderContext()
{
    assert(m_contents.empty() && m_contentCache.empty());
    updateImplementation({});
}

void RenderView::visualizationSelectionChangedEvent(const VisualizationSelection & selection)
{
    updateGuiForSelectedData(selection.visualization);
}

void RenderView::axesEnabledChangedEvent(bool enabled)
{
    implementation().setAxesVisibility(enabled && !m_contents.empty());
}

void RenderView::updateImplementation(const QList<DataObject *> & contents)
{
    implementation().deactivate(qvtkWidget());

    assert(m_contents.empty());
    m_contentCache.clear();
    m_dataObjectToVisualization.clear();

    m_implementationSwitch->findSuitableImplementation(contents);

    implementation().activate(qvtkWidget());

    emit implementationChanged();
}

AbstractVisualizedData * RenderView::addDataObject(DataObject * dataObject)
{
    updateTitle(dataObject->name() + " (loading to GPU)");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // abort if we received a close event in QApplication::processEvents 
    if (m_closingRequested)
    {
        return nullptr;
    }

    if (m_deletedData.remove(dataObject))
    {
        return nullptr;
    }

    auto newContent = implementation().requestVisualization(*dataObject);

    if (!newContent)
    {
        return nullptr;
    }

    auto newContentPtr = newContent.get();
    m_contents.push_back(std::move(newContent));

    m_dataObjectToVisualization.insert(dataObject, newContentPtr);

    implementation().addContent(newContentPtr, 0);

    return newContentPtr;
}

void RenderView::showDataObjectsImpl(const QList<DataObject *> & uncheckedDataObjects, QList<DataObject *> & incompatibleObjects, unsigned int /*subViewIndex*/)
{
    if (uncheckedDataObjects.isEmpty())
    {
        return;
    }

    const bool wasEmpty = m_contents.empty();

    if (wasEmpty)
    {
        updateImplementation(uncheckedDataObjects);
    }

    AbstractVisualizedData * aNewObject = nullptr;

    const auto && dataObjects = implementation().filterCompatibleObjects(uncheckedDataObjects, incompatibleObjects);

    if (dataObjects.isEmpty())
    {
        return;
    }

    for (auto dataObject : dataObjects)
    {
        auto previouslyRendered = m_dataObjectToVisualization.value(dataObject);

        // create new rendered representation
        if (!previouslyRendered)
        {
            aNewObject = addDataObject(dataObject);

            if (m_closingRequested) // just abort here, if we processed a close event while addDataObject()
            {
                return;
            }

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

    resetFriendlyName();
    updateTitle();
}

void RenderView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, int /*suViewIndex*/)
{
    bool changed = false;
    for (auto dataObject : dataObjects)
    {
        auto rendered = m_dataObjectToVisualization.value(dataObject);
        if (!rendered)
        {
            continue;
        }

        // cached data is only accessible internally in the view, so let others know that it's gone for the moment
        emit beforeDeleteVisualizations({ rendered });

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
    {
        return;
    }

    resetFriendlyName();

    updateGuiForRemovedData();

    implementation().renderViewContentsChanged();

    emit visualizationsChanged();

    render();
}

QList<DataObject *> RenderView::dataObjectsImpl(int /*subViewIndex*/) const
{
    QList<DataObject *> dataObjects;
    for (auto && vis : m_contents)
    {
        dataObjects << &vis->dataObject();
    }

    return dataObjects;
}

void RenderView::removeDataObject(DataObject * dataObject)
{
    AbstractVisualizedData * renderedData = m_dataObjectToVisualization.value(dataObject, nullptr);

    // for the case that we are currently loading this object
    m_deletedData << dataObject;

    // we didn't render this object
    if (!renderedData)
    {
        return;
    }

    implementation().removeContent(renderedData, 0);

    emit beforeDeleteVisualizations({ renderedData });

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
    {
        removeDataObject(dataObject);
    }
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

    resetFriendlyName();

    return toDelete;
}

QList<AbstractVisualizedData *> RenderView::visualizationsImpl(int /*subViewIndex*/) const
{
    QList<AbstractVisualizedData *> vis;
    for (auto & content : m_contents)
    {
        vis << content.get();
    }

    return vis;
}

void RenderView::lookAtData(const DataSelection & selection, int subViewIndex)
{
    auto vis = m_dataObjectToVisualization.value(selection.dataObject, nullptr);
    if (!vis)
    {
        return;
    }

    lookAtData(VisualizationSelection(
        selection, 
        vis,
        vis->defaultOutputPort()), // TODO how to find the correct visualization output port?
        subViewIndex);
}

void RenderView::lookAtData(const VisualizationSelection & selection, int DEBUG_ONLY(subViewIndex))
{
    assert(subViewIndex == 0 || subViewIndex == -1);
    assert(containsUnique(m_contents, selection.visualization));

    implementation().lookAtData(selection, 0u);
}

AbstractVisualizedData * RenderView::visualizationFor(DataObject * dataObject, int subViewIndex) const
{
    if (subViewIndex != -1 && subViewIndex != 0)
    {
        return nullptr;
    }

    auto vis = m_dataObjectToVisualization.value(dataObject, nullptr);
    // If it's not visible, it's only cached. In this case, it should not be passed to the public interface.
    if (!vis || !vis->isVisible())
    {
        return nullptr;
    }

    return vis;
}

int RenderView::subViewContaining(const AbstractVisualizedData & visualizedData) const
{
    // single view implementation: if we currently show it, it's in the view 0

    if (containsUnique(m_contents, &visualizedData))
    {
        return 0;
    }

    return -1;
}

RendererImplementation & RenderView::implementation() const
{
    return m_implementationSwitch->currentImplementation();
}

void RenderView::updateGuiForSelectedData(AbstractVisualizedData * renderedData)
{
    updateTitle();

    if (visualzationSelection().visualization != renderedData)
    {
        if (renderedData)
        {
            setVisualizationSelection(VisualizationSelection(renderedData));
        }
        else
        {
            clearSelection();
        }
    }
}

void RenderView::updateGuiForRemovedData()
{
    auto nextSelection = m_contents.empty()
        ? static_cast<AbstractVisualizedData *>(nullptr) : m_contents.front().get();
    
    updateGuiForSelectedData(nextSelection);
}
