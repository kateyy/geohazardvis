#include "DataMapping.h"

#include <algorithm>
#include <cassert>

#include <QCoreApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QPointer>

#include <core/data_objects/DataObject.h>
#include <core/utility/memory.h>
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
    // Close all views and give Qt the chance to correctly handle its hierarchies of ownerships.
    // E.g., when closing a 2D view with an active plot preview window, both views should be closed
    // after triggering closing of the original view.
    // After the first two loops, the qptr*s should be nullptr.
    // However, if the relevant event loop is not running anymore, the views won't be deleted here
    // (all qpointers remain valid). This happens at application shutdown (MainWindow destructor)
    // and in test setups and is highly inconvenient in this case (cannot guarantee that all objects
    // are cleaned up etc.)
    // So, finally, it is save to "force" delete all views here at the end. (Given that DataMapping
    // is not delete within a Qt slot execution).
    std::vector<QPointer<AbstractRenderView>> qptrRenderViews;
    std::vector<QPointer<TableView>> qptrTableViews;
    for (auto && renderView : m_renderViews)
    {
        qptrRenderViews.emplace_back(renderView.get());
    }
    for (auto && tableView : m_tableViews)
    {
        qptrTableViews.emplace_back(tableView.get());
    }

    for (auto && renderView : qptrRenderViews)
    {
        if (renderView)
        {
            renderViewDeleteLater(renderView);
        }
    }

    for (auto && tableView : qptrTableViews)
    {
        if (tableView)
        {
            tableViewDeleteLater(tableView);
        }
    }

    QCoreApplication::processEvents();

    for (auto && renderView : qptrRenderViews)
    {
        if (renderView)
        {
            delete renderView;
        }
    }

    for (auto && tableView : qptrTableViews)
    {
        if (tableView)
        {
            delete tableView;
        }
    }
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
    const auto currentRenderViews = [this] ()
    {
        std::vector<AbstractRenderView *> views;
        for (auto && it : m_renderViews)
        {
            views.push_back(it.get());
        }
        return views;
    }();
    const auto currentTableViews = [this] ()
    {
        std::vector<TableView *> views;
        for (auto && it : m_tableViews)
        {
            views.push_back(it.get());
        }
        return views;
    }();


    for (auto renderView : currentRenderViews)
    {
        if (containsUnique(m_renderViews, renderView))
        {
            renderView->prepareDeleteData(dataObjects);
        }

    }

    for (auto tableView : currentTableViews)
    {
        if (containsUnique(m_tableViews, tableView)
            && dataObjects.contains(tableView->dataObject()))
        {
            tableView->close();
        }
    }
}

void DataMapping::openInTable(DataObject * dataObject)
{
    if (!dataObject)
    {
        return;
    }

    // Open a new table only if there isn't already a table containing dataObject.
    const auto tableIt = std::find_if(m_tableViews.begin(), m_tableViews.end(),
        [dataObject] (const decltype(m_tableViews)::value_type & view)
    {
        return view->dataObject() == dataObject;
    });
    auto table = tableIt != m_tableViews.end() ? tableIt->get() : static_cast<TableView *>(nullptr);

    if (!table)
    {
        auto ownedTable = std::make_unique<TableView>(*this, m_nextTableIndex++);
        table = ownedTable.get();
        m_tableViews.push_back(std::move(ownedTable));
        connect(table, &TableView::closed, this, &DataMapping::tableClosed);

        table->showDataObject(*dataObject);

        // find the first existing docked table, and tabify with it
        QDockWidget * tabMaster = nullptr;
        for (auto && other : m_tableViews)
        {
            if (other->hasDockWidgetParent())
            {
                tabMaster = other->dockWidgetParent();
                assert(tabMaster);
                break;
            }
        }

        emit tableViewCreated(table, tabMaster);

        m_selectionHandler->addTableView(table);
    }
    
    QCoreApplication::processEvents();
}

AbstractRenderView * DataMapping::openInRenderView(const QList<DataObject *> & dataObjects)
{
    auto renderView = createRenderView<RenderView>();

    const bool viewStillOpen = addToRenderView(dataObjects, renderView);

    if (!viewStillOpen)
    {
        return nullptr;
    }

    setFocusedRenderView(renderView);

    return renderView;
}

bool DataMapping::addToRenderView(const QList<DataObject *> & dataObjects, AbstractRenderView * renderView, unsigned int subViewIndex)
{
    assert(containsUnique(m_renderViews, renderView));
    assert(subViewIndex < renderView->numberOfSubViews());

    QList<DataObject *> incompatibleObjects;
    renderView->showDataObjects(dataObjects, incompatibleObjects, subViewIndex);

    // RenderView::showDataObjects triggers QApplication::processEvents(), so the view might be closed again
    // in that case, the user probably wants to abort his last action or close the app
    // so abort everything from here
    if (!containsUnique(m_renderViews, renderView))
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

AbstractRenderView * DataMapping::createDefaultRenderViewType()
{
    auto renderView = createRenderView<RenderView>();

    setFocusedRenderView(renderView);

    return renderView;
}

AbstractRenderView * DataMapping::focusedRenderView()
{
    return m_focusedRenderView;
}

QList<AbstractRenderView *> DataMapping::renderViews() const
{
    decltype(renderViews()) list;
    for (auto && it : m_renderViews)
    {
        list.push_back(it.get());
    }
    return list;
}

QList<TableView *> DataMapping::tableViews() const
{
    decltype(tableViews()) list;
    for (auto && it : m_tableViews)
    {
        list.push_back(it.get());
    }
    return list;
}

void DataMapping::setFocusedRenderView(AbstractDataView * renderView)
{
    if (m_focusedRenderView == renderView)
    {
        return;
    }

    if (renderView && !renderView->isRenderer())
    {
        return;
    }

    // check if the user clicked on a view that we are currently closing
    if (renderView && renderView->isClosed())
    {
        focusNextRenderView();
        return;
    }

    if (m_focusedRenderView)
    {
        m_focusedRenderView->setCurrent(false);
    }

    assert(!renderView || dynamic_cast<AbstractRenderView *>(renderView));
    m_focusedRenderView = static_cast<AbstractRenderView *>(renderView);

    if (m_focusedRenderView)
    {
        m_focusedRenderView->setCurrent(true);
        m_focusedRenderView->setFocus();
    }

    emit focusedRenderViewChanged(m_focusedRenderView);
}

void DataMapping::focusNextRenderView()
{
    auto nextViewIt = std::find_if(m_renderViews.begin(), m_renderViews.end(),
    [this] (const decltype(m_renderViews)::value_type & view)
    {
        return (m_focusedRenderView != view.get()) && !view->isClosed();
    });

    auto nextView = nextViewIt != m_renderViews.end()
        ? nextViewIt->get()
        : static_cast<decltype(nextViewIt->get())>(nullptr);

    setFocusedRenderView(nextView);
}

void DataMapping::tableClosed()
{
    assert(dynamic_cast<TableView *>(sender()));
    auto table = static_cast<TableView *>(sender());

    m_selectionHandler->removeTableView(table);

    tableViewDeleteLater(table);
}

void DataMapping::renderViewClosed()
{
    assert(dynamic_cast<AbstractRenderView *>(sender()));
    auto renderView = static_cast<AbstractRenderView *>(sender());

    m_selectionHandler->removeRenderView(renderView);

    if (renderView == m_focusedRenderView)
    {
        focusNextRenderView();
    }

    renderViewDeleteLater(renderView);
}

void DataMapping::renderViewDeleteLater(AbstractRenderView * view)
{
    deleteLaterFrom(view, m_renderViews);
}

void DataMapping::tableViewDeleteLater(TableView * view)
{
    deleteLaterFrom(view, m_tableViews);
}

template<typename View_t, typename Vector_t>
void DataMapping::deleteLaterFrom(View_t * view, Vector_t & vector)
{
    auto ownedViewIt = findUnique(vector, view);
    if (ownedViewIt == vector.end())
    {
        // Reentered this function while calling view->dockWidgetParent()->close();
        return;
    }

    auto viewOwnership = std::move(*ownedViewIt);
    vector.erase(ownedViewIt);

    // Handle deletion in Qt event loop. If the event loop is not running anymore (application shutdown),
    // deletion will be handled in ~DataMapping
    viewOwnership.release()->deleteLater();
}

void DataMapping::addRenderView(std::unique_ptr<AbstractRenderView> ownedRenderView)
{
    assert(ownedRenderView);
    auto renderView = ownedRenderView.get();

    m_renderViews.push_back(std::move(ownedRenderView));

    connect(renderView, &AbstractDataView::focused, this, &DataMapping::setFocusedRenderView);
    connect(renderView, &AbstractDataView::closed, this, &DataMapping::renderViewClosed);

    m_selectionHandler->addRenderView(renderView);

    emit renderViewCreated(renderView);
}

bool DataMapping::askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects)
{
    QString msg = "Cannot add some data to the current render view (" + rendererName + "):\n";
    for (auto object : relevantObjects)
    {
        msg += object->name() + ", ";
    }
    msg.chop(2);
    msg += "\n\n";
    msg += "Do you want to open them in a new view?";

    return QMessageBox(QMessageBox::Question, "", msg, QMessageBox::Yes | QMessageBox::No).exec() == QMessageBox::Yes;
}
