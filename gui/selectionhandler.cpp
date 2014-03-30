#include "selectionhandler.h"

#include <cassert>

#include <QTableView>

#include <vtkDataObject.h>

#include "pickinginteractionstyle.h"

SelectionHandler::SelectionHandler()
: m_tableView(nullptr)
, m_tableModel(nullptr)
, m_interactionStyle(nullptr)
{
}

void SelectionHandler::setQtTableView(QTableView * tableView, QVtkTableModel * model)
{
    m_tableView = tableView;
    m_tableModel = model;
    createConnections();
}

void SelectionHandler::setVtkInteractionStyle(PickingInteractionStyle * interactionStyle)
{
    m_interactionStyle = interactionStyle;
    createConnections();
}

void SelectionHandler::setDataObject(vtkDataObject * dataObject)
{
    m_dataObject = dataObject;
}

void SelectionHandler::createConnections()
{
    if (!(m_tableView && m_tableModel && m_interactionStyle))
        return;

    connect(m_interactionStyle, &PickingInteractionStyle::selectionChanged, m_tableView, &QTableView::selectRow);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SelectionHandler::qtSelectionChanged);
}

void SelectionHandler::qtSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    if (m_dataObject == nullptr)
        return;

    if (selected.indexes().isEmpty())
        return;

    m_interactionStyle->highlightCell(selected.indexes().first().row(), m_dataObject);
}
