#include "DataMapping.h"

#include <cassert>

#include <QMessageBox>

#include "core/Input.h"

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
    TableWidget * table = new TableWidget(m_nextTableIndex++);
    m_tableWidgets.insert(table->index(), table);

    connect(table, &TableWidget::closed, this, &DataMapping::tableClosed);

    m_mainWindow.addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, table);

    table->setModel(representation->tableModel());
}

void DataMapping::openInRenderView(std::shared_ptr<InputRepresentation> representation)
{
    RenderWidget * renderWidget = m_mainWindow.addRenderWidget();
    m_renderWidgets.insert(m_nextRenderWidgetIndex++, renderWidget);

    std::shared_ptr<Input> input = representation->input();

    switch (input->type) {
    case ModelType::triangles:
        renderWidget->show3DInput(std::dynamic_pointer_cast<PolyDataInput>(input));
        break;
    case ModelType::grid2d:
        renderWidget->showGridInput(std::dynamic_pointer_cast<GridDataInput>(input));
        break;
    default:
        assert(false);
        return;
    }
}

void DataMapping::addToRenderView(std::shared_ptr<InputRepresentation> representation, int renderView)
{
    QMessageBox::warning(&m_mainWindow, "nan", "niy");
}

void DataMapping::tableClosed()
{
    TableWidget * table = dynamic_cast<TableWidget*>(sender());
    assert(table);

    m_tableWidgets.remove(table->index());
    table->deleteLater();
}
