#include "RenderView.h"
#include "ui_RenderView.h"

#include <cassert>

#include <QMessageBox>

#include <vtkRenderWindow.h>

#include <core/types.h>
#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/macros.h>
#include <core/utility/memory.h>

#include <gui/data_view/RendererImplementation.h>
#include <gui/data_view/RendererImplementationSwitch.h>
#include <gui/SelectionHandler.h>


RenderView::RenderView(
    int index,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(index, parent, flags)
    , m_ui(std::make_unique<Ui_RenderView>())
    , m_implementationSwitch(std::make_unique<RendererImplementationSwitch>(*this))
    , m_closingRequested(false)
{
    m_ui->setupUi(this);

    updateTitle();

    SelectionHandler::instance().addRenderView(this);

    updateImplementation({});
}

RenderView::~RenderView()
{
    SelectionHandler::instance().removeRenderView(this); 

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
    implementation().setAxesVisibility(enabled && !m_contents.empty());
}

void RenderView::updateImplementation(const QList<DataObject *> & contents)
{
    implementation().deactivate(m_ui->qvtkMain);

    disconnect(&implementation(), &RendererImplementation::dataSelectionChanged,
        this, &RenderView::updateGuiForSelectedData);

    assert(m_contents.empty());
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
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // abort if we received a close event in QApplication::processEvents 
    if (m_closingRequested)
        return nullptr;

    if (m_deletedData.remove(dataObject))
        return nullptr;

    assert(dataObject->is3D() == (contentType() == ContentType::Rendered3D));

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
        assert(findUnique(m_contents, (AbstractVisualizedData*)nullptr) == m_contents.end());
        assert(findUnique(m_contents, (AbstractVisualizedData*)nullptr) == m_contents.end());
    }

    if (!changed)
        return;

    updateGuiForRemovedData();

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
    auto selected = implementation().selectedData();

    if (!selected && !m_contents.empty())
        selected = &m_contents.front()->dataObject();

    return selected;
}

AbstractVisualizedData * RenderView::selectedDataVisualization() const
{
    return m_dataObjectToVisualization.value(selectedData());
}

void RenderView::lookAtData(DataObject * dataObject, vtkIdType itemId, int DEBUG_ONLY(subViewIndex))
{
    assert(subViewIndex == 0);
    implementation().lookAtData(dataObject, itemId, 0);
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
