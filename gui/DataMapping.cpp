#include "DataMapping.h"

#include <cassert>

#include <QCoreApplication>
#include <QMessageBox>

#include <core/data_objects/DataObject.h>

#include <gui/MainWindow.h>
#include <gui/data_view/TableView.h>
#include <gui/data_view/RenderView.h>


namespace
{
    DataMapping * s_instance = nullptr;
}

DataMapping::DataMapping(MainWindow & mainWindow)
    : m_mainWindow(mainWindow)
    , m_nextTableIndex(0)
    , m_nextRenderViewIndex(0)
    , m_focusedRenderView(nullptr)
{
    assert(!s_instance);
    s_instance = this;
}

DataMapping::~DataMapping()
{
    assert(s_instance);

    // prevent GUI/focus updates
    auto renderView = m_renderViews;
    auto tableViews = m_tableViews;
    m_renderViews.clear();
    m_tableViews.clear();

    qDeleteAll(renderView);
    qDeleteAll(tableViews);

    s_instance = nullptr;
}

DataMapping & DataMapping::instance()
{
    assert(s_instance);
    return *s_instance;
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
        m_mainWindow.addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, table->dockWidgetParent());

        // find the first existing docked table, and tabify with it
        QDockWidget * tabMaster = nullptr;
        for (auto other : m_tableViews)
        {
            if (other->hasDockWidgetParent())
            {
                tabMaster = other->dockWidgetParent();
                assert(tabMaster);
            }
        }

        if (tabMaster)
        {
            m_mainWindow.tabifyDockWidget(tabMaster, table->dockWidgetParent());
        }

        connect(table, &TableView::focused, this, &DataMapping::setFocusedView);

        m_tableViews.insert(table->index(), table);
    }

    if (m_tableViews.size() > 1)
    {
        QCoreApplication::processEvents(); // setup GUI before searching for the tabbed widget...
        m_mainWindow.tabbedDockWidgetToFront(table->dockWidgetParent());
    }
    
    QCoreApplication::processEvents();

    table->showDataObject(dataObject);
}

RenderView * DataMapping::openInRenderView(QList<DataObject *> dataObjects)
{
    RenderView * renderView = new RenderView(m_nextRenderViewIndex++);
    m_mainWindow.addRenderView(renderView);
    m_renderViews.insert(renderView->index(), renderView);

    connect(renderView, &RenderView::focused, this, &DataMapping::setFocusedView);
    connect(renderView, &RenderView::closed, this, &DataMapping::renderViewClosed);

    addToRenderView(dataObjects, renderView);

    return renderView;
}

void DataMapping::addToRenderView(QList<DataObject *> dataObjects, RenderView * renderView)
{
    assert(m_renderViews.key(renderView, -1) >= 0);

    QList<DataObject *> incompatibleObjects;
    renderView->addDataObjects(dataObjects, incompatibleObjects);

    // there is something the current view couldn't handle
    if (!incompatibleObjects.isEmpty() && askForNewRenderView(renderView->friendlyName(), incompatibleObjects))
    {
        openInRenderView(incompatibleObjects);
    }

    setFocusedView(renderView);

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
        {
            m_focusedRenderView->setCurrent(true);
            m_focusedRenderView->setFocus();
        }

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

bool DataMapping::askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects)
{
    QString msg = "Cannot add some data to the current render view (" + rendererName + "):\n";
    for (DataObject * object : relevantObjects)
        msg += object->name() + ", ";
    msg.chop(2);
    msg += "\n\n";
    msg += "Should we try to open these in a new view?";

    return QMessageBox(QMessageBox::Question, m_mainWindow.windowTitle(), msg, QMessageBox::Yes | QMessageBox::No, &m_mainWindow).exec() == QMessageBox::Yes;
}
