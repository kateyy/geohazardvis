#include "DataMapping.h"

#include <cassert>

#include <QMessageBox>

#include "MainWindow.h"
#include "InputRepresentation.h"
#include "widgets/TableWidget.h"
#include "widgets/RenderWidget.h"


DataMapping::DataMapping(MainWindow & mainWindow)
    : m_mainWindow(mainWindow)
    , m_nextTableIndex(0)
    , m_nextRenderWidgetIndex(0)
{
}

void DataMapping::addInputRepresenation(std::shared_ptr<InputRepresentation> input)
{
    m_inputRepresentations << input;
}

void DataMapping::openInTable(std::shared_ptr<InputRepresentation> representation)
{
    TableWidget * table = nullptr;
    for (TableWidget * existingTable : m_tableWidgets)
    {
        if (existingTable->input() == representation)
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

    table->showInput(representation);
}

void DataMapping::openInRenderView(std::shared_ptr<InputRepresentation> representation)
{
    RenderWidget * renderWidget = m_mainWindow.addRenderWidget(m_nextRenderWidgetIndex++);
    m_renderWidgets.insert(renderWidget->index(), renderWidget);
    connect(renderWidget, &RenderWidget::closed, this, &DataMapping::renderWidgetClosed);

    renderWidget->setObject(representation);

    emit renderViewsChanged(m_renderWidgets.values());
}

void DataMapping::addToRenderView(std::shared_ptr<InputRepresentation> representation, int renderView)
{
    assert(m_renderWidgets.contains(renderView));
    m_renderWidgets[renderView]->addObject(representation);

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
