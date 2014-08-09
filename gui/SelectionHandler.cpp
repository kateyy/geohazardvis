#include "SelectionHandler.h"

#include <cassert>

#include <QTableView>

#include <core/data_objects/DataObject.h>

#include "widgets/TableWidget.h"
#include "widgets/RenderWidget.h"
#include "IPickingInteractorStyle.h"


SelectionHandler & SelectionHandler::instance()
{
    static SelectionHandler selectionHandler;
    return selectionHandler;
}

SelectionHandler::SelectionHandler()
{
}

SelectionHandler::~SelectionHandler() = default;

void SelectionHandler::addTableView(TableWidget * tableWidget)
{
    m_tableWidgets << tableWidget;
    connect(tableWidget, &TableWidget::cellSelected, this, &SelectionHandler::tableSelectionChanged);
}

void SelectionHandler::addRenderView(RenderWidget * renderWidget)
{
    m_renderWidgets << renderWidget;
    connect(renderWidget->interactorStyle(), &IPickingInteractorStyle::cellPicked, this, &SelectionHandler::cellPicked);
}

void SelectionHandler::removeTableView(TableWidget * tableWidget)
{
    m_tableWidgets.remove(tableWidget);
}

void SelectionHandler::removeRenderView(RenderWidget * renderWidget)
{
    m_renderWidgets.remove(renderWidget);
}

void SelectionHandler::tableSelectionChanged(int cellId)
{
    TableWidget * table = dynamic_cast<TableWidget*>(sender());
    assert(table);

    DataObject * dataObject = table->dataObject();

    for (RenderWidget * renderWidget : m_renderWidgets)
    {
        if (renderWidget->dataObjects().contains(dataObject))
            renderWidget->interactorStyle()->highlightCell(cellId, dataObject);
    }
}

void SelectionHandler::cellPicked(DataObject * dataObject, vtkIdType cellId)
{
    for (RenderWidget * renderWidget : m_renderWidgets)
    {
        if (renderWidget->dataObjects().contains(dataObject))
            renderWidget->interactorStyle()->highlightCell(cellId, dataObject);
    }
    for (TableWidget * table : m_tableWidgets)
    {
        if (table->dataObject() == dataObject)
            table->selectCell(cellId);
    }
}
