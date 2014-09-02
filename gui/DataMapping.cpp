#include "DataMapping.h"

#include <cassert>

#include <QMessageBox>
#include <QCoreApplication>

#include "core/data_objects/DataObject.h"

#include "MainWindow.h"
#include "widgets/TableWidget.h"
#include "widgets/RenderWidget.h"


DataMapping::DataMapping(MainWindow & mainWindow)
    : m_mainWindow(mainWindow)
    , m_nextTableIndex(0)
    , m_nextRenderWidgetIndex(0)
    , m_focusedRenderView(nullptr)
{
}

DataMapping::~DataMapping()
{
    qDeleteAll(m_renderWidgets.values());
    qDeleteAll(m_tableWidgets.values());
}

void DataMapping::addDataObjects(QList<DataObject *> dataObjects)
{
    m_dataObject << dataObjects;
}

void DataMapping::removeDataObjects(QList<DataObject *> dataObjects)
{
    QList<RenderWidget*> currentRenderWidgets = m_renderWidgets.values();
    for (RenderWidget * renderWidget : currentRenderWidgets)
        renderWidget->removeDataObjects(dataObjects);

    QList<TableWidget*> currentTableWidgets = m_tableWidgets.values();
    for (TableWidget * tableWidget : currentTableWidgets)
    {
        if (dataObjects.contains(tableWidget->dataObject()))
            tableWidget->close();
    }
}

void DataMapping::openInTable(DataObject * dataObject)
{
    TableWidget * table = nullptr;
    for (TableWidget * existingTable : m_tableWidgets)
    {
        if (existingTable->dataObject() == dataObject)
        {
            table = existingTable;
            break;
        }
    }

    if (!table)
    {
        table = new TableWidget(m_nextTableIndex++);
        connect(table, &TableWidget::closed, this, &DataMapping::tableClosed);
        m_mainWindow.addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, table);

        if (!m_tableWidgets.empty())
        {
            m_mainWindow.tabifyDockWidget(m_tableWidgets.first(), table);
        }

        connect(table, &TableWidget::focused, this, &DataMapping::setFocusedTableView);

        m_tableWidgets.insert(table->index(), table);
    }

    if (m_tableWidgets.size() > 1)
    {
        QCoreApplication::processEvents(); // setup GUI before searching for the tabbed widget...
        m_mainWindow.tabbedDockWidgetToFront(table);
    }

    table->showInput(dataObject);
}

void DataMapping::openInRenderView(QList<DataObject *> dataObjects)
{
    RenderWidget * renderWidget = m_mainWindow.addRenderWidget(m_nextRenderWidgetIndex++);
    m_renderWidgets.insert(renderWidget->index(), renderWidget);

    connect(renderWidget, &RenderWidget::focused, this, &DataMapping::setFocusedRenderView);
    connect(renderWidget, &RenderWidget::closed, this, &DataMapping::renderWidgetClosed);

    renderWidget->addDataObjects(dataObjects);

    emit renderViewsChanged(m_renderWidgets.values());
}

void DataMapping::addToRenderView(QList<DataObject *> dataObjects, int renderView)
{
    assert(m_renderWidgets.contains(renderView));
    m_renderWidgets[renderView]->addDataObjects(dataObjects);

    emit renderViewsChanged(m_renderWidgets.values());
}

RenderWidget * DataMapping::focusedRenderView()
{
    return m_focusedRenderView;
}

void DataMapping::setFocusedRenderView(RenderWidget * renderView)
{
    m_focusedRenderView = renderView;

    emit focusedRenderViewChanged(renderView);
}

void DataMapping::setFocusedTableView(TableWidget * /*tableView*/)
{
    m_focusedRenderView = nullptr;

    emit focusedRenderViewChanged(nullptr);
}

void DataMapping::tableClosed()
{
    TableWidget * table = dynamic_cast<TableWidget*>(sender());
    assert(table);

    m_tableWidgets.remove(table->index());
    table->deleteLater();
}

void DataMapping::renderWidgetClosed()
{
    RenderWidget * renderWidget = dynamic_cast<RenderWidget*>(sender());
    assert(renderWidget);

    m_renderWidgets.remove(renderWidget->index());
    renderWidget->deleteLater();

    emit renderViewsChanged(m_renderWidgets.values());
}
