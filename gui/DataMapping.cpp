#include "DataMapping.h"

#include <cassert>

#include <QCoreApplication>

#include <core/data_objects/DataObject.h>

#include <gui/MainWindow.h>
#include "data_view/TableView.h"
#include "data_view/RenderView.h"


DataMapping::DataMapping(MainWindow & mainWindow)
    : m_mainWindow(mainWindow)
    , m_nextTableIndex(0)
    , m_nextRenderViewIndex(0)
    , m_focusedRenderView(nullptr)
{
}

DataMapping::~DataMapping()
{
    qDeleteAll(m_renderViews.values());
    qDeleteAll(m_tableViews.values());
}

void DataMapping::removeDataObjects(QList<DataObject *> dataObjects)
{
    QList<RenderView*> currentRenderViews = m_renderViews.values();
    for (RenderView * renderView : currentRenderViews)
        renderView->removeDataObjects(dataObjects);

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
        table = new TableView(m_nextTableIndex++);
        connect(table, &TableView::closed, this, &DataMapping::tableClosed);
        m_mainWindow.addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, table);

        if (!m_tableViews.empty())
        {
            m_mainWindow.tabifyDockWidget(m_tableViews.first(), table);
        }

        connect(table, &TableView::focused, this, &DataMapping::setFocusedView);
        connect(table, &TableView::dataChanged, this, &DataMapping::updateRendererForChangedTable);

        m_tableViews.insert(table->index(), table);
    }

    if (m_tableViews.size() > 1)
    {
        QCoreApplication::processEvents(); // setup GUI before searching for the tabbed widget...
        m_mainWindow.tabbedDockWidgetToFront(table);
    }

    table->showDataObject(dataObject);
}

RenderView * DataMapping::openInRenderView(QList<DataObject *> dataObjects)
{
    RenderView * renderView = m_mainWindow.addRenderView(m_nextRenderViewIndex++);
    m_renderViews.insert(renderView->index(), renderView);

    connect(renderView, &RenderView::focused, this, &DataMapping::setFocusedView);
    connect(renderView, &RenderView::closed, this, &DataMapping::renderViewClosed);

    renderView->addDataObjects(dataObjects);

    setFocusedView(renderView);

    emit renderViewsChanged(m_renderViews.values());

    return renderView;
}

void DataMapping::addToRenderView(QList<DataObject *> dataObjects, int renderView)
{
    assert(m_renderViews.contains(renderView));
    RenderView * view = m_renderViews[renderView];
    view->addDataObjects(dataObjects);

    setFocusedView(view);

    emit renderViewsChanged(m_renderViews.values());
}

RenderView * DataMapping::focusedRenderView()
{
    return m_focusedRenderView;
}

void DataMapping::setFocusedView(AbstractDataView * view)
{
    if (view->isRenderer())
    {
        if (m_focusedRenderView == view)
            return;

        if (m_focusedRenderView)
            m_focusedRenderView->setCurrent(false);

        m_focusedRenderView = static_cast<RenderView*>(view);

        if (m_focusedRenderView)
            m_focusedRenderView->setCurrent(true);

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

void DataMapping::updateRendererForChangedTable()
{
    TableView * table = dynamic_cast<TableView *>(sender());
    assert(table);

    DataObject * data = table->dataObject();

    for (RenderView * renderer : m_renderViews)
    {
        if (renderer->dataObjects().contains(data))
            renderer->render();
    }
}

void DataMapping::tableClosed()
{
    TableView * table = dynamic_cast<TableView*>(sender());
    assert(table);

    m_tableViews.remove(table->index());
    table->deleteLater();
}

void DataMapping::renderViewClosed()
{
    RenderView * renderView = dynamic_cast<RenderView*>(sender());
    assert(renderView);

    m_renderViews.remove(renderView->index());

    if (renderView == m_focusedRenderView)
        focusNextRenderView();

    renderView->deleteLater();

    emit renderViewsChanged(m_renderViews.values());
}
