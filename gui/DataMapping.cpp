#include "DataMapping.h"

#include <cassert>

#include <QCoreApplication>
#include <QMessageBox>

#include <core/data_objects/DataObject.h>

#include <gui/SelectionHandler.h>
#include <gui/data_view/TableView.h>
#include <gui/data_view/RenderView.h>


DataMapping::DataMapping(DataSetHandler & dataSetHandler)
    : m_dataSetHandler{ dataSetHandler }
    , m_selectionHandler{ std::make_unique<SelectionHandler>() }
    , m_nextTableIndex{ 0 }
    , m_nextRenderViewIndex{ 0 }
    , m_focusedRenderView{ nullptr }
{
}

DataMapping::~DataMapping()
{
    // prevent GUI/focus updates
    auto renderView = m_renderViews;
    auto tableViews = m_tableViews;
    m_renderViews.clear();
    m_tableViews.clear();

    qDeleteAll(renderView);
    qDeleteAll(tableViews);
}

DataSetHandler & DataMapping::dataSetHandler() const
{
    return m_dataSetHandler;
}

SelectionHandler & DataMapping::selectionHandler()
{
    assert(m_selectionHandler);
    return *m_selectionHandler;
}

void DataMapping::removeDataObjects(const QList<DataObject *> & dataObjects)
{
    // copy, as this list can change while we are processing it
    // e.g., deleting an image -> delete related plots -> close related plot renderer
    auto currentRenderViews = m_renderViews.values();
    for (AbstractRenderView * renderView : currentRenderViews)
    {
        if (!m_renderViews.values().contains(renderView))
            continue;

        renderView->prepareDeleteData(dataObjects);
    }

    QList<TableView*> currentTableViews = m_tableViews.values();
    for (TableView * tableView : currentTableViews)
    {
        if (dataObjects.contains(tableView->dataObject()))
            tableView->close();
    }
}

void DataMapping::openInTable(DataObject * dataObject)
{
    TableView * table = nullptr;
    for (TableView * existingTable : m_tableViews)
    {
        if (existingTable->dataObject() == dataObject)
        {
            table = existingTable;
            break;
        }
    }

    if (!table)
    {
        table = new TableView(*this, m_nextTableIndex++);
        connect(table, &TableView::closed, this, &DataMapping::tableClosed);

        // find the first existing docked table, and tabify with it
        QDockWidget * tabMaster = nullptr;
        for (auto other : m_tableViews)
        {
            if (other->hasDockWidgetParent())
            {
                tabMaster = other->dockWidgetParent();
                assert(tabMaster);
                break;
            }
        }

        emit tableViewCreated(table, tabMaster);

        connect(table, &TableView::focused, this, &DataMapping::setFocusedView);

        m_tableViews.insert(table->index(), table);

        m_selectionHandler->addTableView(table);
    }
    
    QCoreApplication::processEvents();

    table->showDataObject(dataObject);
}

AbstractRenderView * DataMapping::openInRenderView(QList<DataObject *> dataObjects)
{
    auto renderView = createRenderView<RenderView>();

    bool viewStillOpen = addToRenderView(dataObjects, renderView);

    if (!viewStillOpen)
        return nullptr;

    setFocusedView(renderView);

    return renderView;
}

bool DataMapping::addToRenderView(const QList<DataObject *> & dataObjects, AbstractRenderView * renderView, unsigned int subViewIndex)
{
    assert(m_renderViews.key(renderView, -1) >= 0);
    assert(subViewIndex < renderView->numberOfSubViews());

    QList<DataObject *> incompatibleObjects;
    renderView->showDataObjects(dataObjects, incompatibleObjects, subViewIndex);

    // RenderView::showDataObjects triggers QApplication::processEvents(), so the view might be closed again
    // in that case, the user probably wants to abort his last action or close the app
    // so abort everything from here
    if (!m_renderViews.values().contains(renderView))
    {
        return false;
    }

    // there is something the current view couldn't handle
    if (!incompatibleObjects.isEmpty() && askForNewRenderView(renderView->friendlyName(), incompatibleObjects))
    {
        openInRenderView(incompatibleObjects);
    }

    return true;
}

AbstractRenderView * DataMapping::focusedRenderView()
{
    return m_focusedRenderView;
}

QList<AbstractRenderView *> DataMapping::renderViews() const
{
    return m_renderViews.values();
}

QList<TableView *> DataMapping::tableViews() const
{
    return m_tableViews.values();
}

void DataMapping::setFocusedView(AbstractDataView * view)
{
    assert(view);

    if (view->isRenderer())
    {
        if (m_focusedRenderView == view)
            return;

        if (m_focusedRenderView)
            m_focusedRenderView->setCurrent(false);

        assert(dynamic_cast<AbstractRenderView *>(view));
        m_focusedRenderView = static_cast<AbstractRenderView *>(view);

        // check if the user clicked on a view that we are currently closing
        if (m_renderViews.value(view->index()) != view) 
        {
            assert(!view->isVisible());
            focusNextRenderView();
            return;
        }

        m_focusedRenderView->setCurrent(true);
        m_focusedRenderView->setFocus();

        emit focusedRenderViewChanged(m_focusedRenderView);
    }
}

void DataMapping::focusNextRenderView()
{
    if (m_renderViews.isEmpty())
        m_focusedRenderView = nullptr;
    else
    {
        m_focusedRenderView = m_renderViews.first();
        m_focusedRenderView->setCurrent(true);
    }

    emit focusedRenderViewChanged(m_focusedRenderView);
}

void DataMapping::tableClosed()
{
    TableView * table = dynamic_cast<TableView*>(sender());
    assert(table);

    m_selectionHandler->removeTableView(table);
    m_tableViews.remove(table->index());
    table->deleteLater();
}

void DataMapping::renderViewClosed()
{
    assert(dynamic_cast<AbstractRenderView *>(sender()));
    auto renderView = static_cast<AbstractRenderView *>(sender());

    m_selectionHandler->removeRenderView(renderView);

    m_renderViews.remove(renderView->index());

    if (renderView == m_focusedRenderView)
        focusNextRenderView();

    renderView->deleteLater();

    emit renderViewsChanged(m_renderViews.values());
}

void DataMapping::addRenderView(AbstractRenderView * renderView)
{
    assert(!m_renderViews.contains(renderView->index()));
    assert(!m_renderViews.values().contains(renderView));

    m_renderViews.insert(renderView->index(), renderView);

    connect(renderView, &AbstractDataView::focused, this, &DataMapping::setFocusedView);
    connect(renderView, &AbstractDataView::closed, this, &DataMapping::renderViewClosed);

    m_selectionHandler->addRenderView(renderView);

    emit renderViewCreated(renderView);

    emit renderViewsChanged(m_renderViews.values());
}

bool DataMapping::askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects)
{
    QString msg = "Cannot add some data to the current render view (" + rendererName + "):\n";
    for (DataObject * object : relevantObjects)
        msg += object->name() + ", ";
    msg.chop(2);
    msg += "\n\n";
    msg += "Should we try to open these in a new view?";

    return QMessageBox(QMessageBox::Question, "", msg, QMessageBox::Yes | QMessageBox::No).exec() == QMessageBox::Yes;
}
