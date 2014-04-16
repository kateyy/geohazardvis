#include "selectionhandler.h"

#include <cassert>

#include <QTableView>

#include <vtkDataObject.h>
#include <vtkPolyData.h>

#include "pickinginteractionstyle.h"

SelectionHandler::SelectionHandler()
: m_tableView(nullptr)
, m_tableModel(nullptr)
, m_interactionStyle(nullptr)
, m_currentSelection(-1)
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

    connect(m_interactionStyle, &PickingInteractionStyle::cellPicked, this, &SelectionHandler::cellPicked);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SelectionHandler::qtSelectionChanged);
}

void SelectionHandler::updateSelections(vtkIdType cellId, bool updateView)
{
    m_currentSelection = cellId;

    m_interactionStyle->highlightCell(cellId, m_dataObject);


    if (updateView) {
        vtkPolyData * polyData = vtkPolyData::SafeDownCast(m_dataObject);
        if (polyData)
            m_interactionStyle->lookAtCell(polyData, cellId);
    }

    m_tableView->selectRow(cellId);
}

void SelectionHandler::cellPicked(vtkDataObject * dataObject, vtkIdType cellId)
{
    // assertion not valid for image/grid input data
    //assert(m_dataObject == dataObject); // not implemented for multiple data objects
    updateSelections(cellId, false);
}

void SelectionHandler::qtSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    if (m_dataObject == nullptr)
        return;

    if (selected.indexes().isEmpty())
        return;

    vtkIdType newSelection = selected.indexes().first().row();
    if (newSelection != m_currentSelection) {
        updateSelections(newSelection, true);
    }
}
