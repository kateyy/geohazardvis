#include "DataMapping.h"

#include <cassert>

#include <QMessageBox>

#include "core/data_objects/DataObject.h"

#include "MainWindow.h"
#include "widgets/TableWidget.h"
#include "widgets/RenderWidget.h"


DataMapping::DataMapping(MainWindow & mainWindow)
    : m_mainWindow(mainWindow)
    , m_nextTableIndex(0)
    , m_nextRenderWidgetIndex(0)
{
}

DataMapping::~DataMapping()
{
    qDeleteAll(m_renderWidgets.values());
    qDeleteAll(m_tableWidgets.values());
}

void DataMapping::addDataObject(DataObject * dataObject)
{
    m_dataObject << dataObject;
}

void DataMapping::removeDataObject(DataObject * dataObject)
{
    QList<RenderWidget*> currentRenderWidgets = m_renderWidgets.values();
    for (RenderWidget * renderWidget : currentRenderWidgets)
        renderWidget->removeDataObject(dataObject);

    QList<TableWidget*> currentTableWidgets = m_tableWidgets.values();
    for (TableWidget * tableWidget : currentTableWidgets)
    {
        if (tableWidget->dataObject() == dataObject)
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

        m_tableWidgets.insert(table->index(), table);
    }

    table->showInput(dataObject);
}

void DataMapping::openInRenderView(DataObject * dataObject)
{
    RenderWidget * renderWidget = m_mainWindow.addRenderWidget(m_nextRenderWidgetIndex++);
    m_renderWidgets.insert(renderWidget->index(), renderWidget);
    connect(renderWidget, &RenderWidget::closed, this, &DataMapping::renderWidgetClosed);

    renderWidget->setDataObject(dataObject);

    emit renderViewsChanged(m_renderWidgets.values());
}

void DataMapping::addToRenderView(DataObject * dataObject, int renderView)
{
    assert(m_renderWidgets.contains(renderView));
    m_renderWidgets[renderView]->addDataObject(dataObject);

    emit renderViewsChanged(m_renderWidgets.values());
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
