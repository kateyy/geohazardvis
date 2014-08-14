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
    connect(tableWidget, &TableWidget::cellSelected, this, &SelectionHandler::syncRenderViewsWithTable);
    connect(tableWidget, &TableWidget::cellDoubleClicked, 
        [this](DataObject * dataObject, int row) {
        renderViewsLookAt(dataObject, static_cast<vtkIdType>(row));
    });
}

void SelectionHandler::addRenderView(RenderWidget * renderWidget)
{
    m_renderWidgets << renderWidget;
    connect(renderWidget->interactorStyle(), &IPickingInteractorStyle::cellPicked, this, &SelectionHandler::syncRenderAndTableViews);
}

void SelectionHandler::removeTableView(TableWidget * tableWidget)
{
    m_tableWidgets.remove(tableWidget);
}

void SelectionHandler::removeRenderView(RenderWidget * renderWidget)
{
    m_renderWidgets.remove(renderWidget);
}

void SelectionHandler::syncRenderViewsWithTable(DataObject * dataObject, vtkIdType cellId)
{
    for (RenderWidget * renderWidget : m_renderWidgets)
    {
        if (renderWidget->dataObjects().contains(dataObject))
            renderWidget->interactorStyle()->highlightCell(cellId, dataObject);
    }
}

void SelectionHandler::syncRenderAndTableViews(DataObject * dataObject, vtkIdType cellId)
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

void SelectionHandler::renderViewsLookAt(DataObject * dataObject, vtkIdType cellId)
{
    for (RenderWidget * renderWidget : m_renderWidgets)
    {
        renderWidget->interactorStyle()->lookAtCell(dataObject, cellId);
    }
}
