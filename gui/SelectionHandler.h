#pragma once

#include <QObject>

#include <vtkSmartPointer.h>


class QTableView;
class QItemSelection;
class QVtkTableModel;
class vtkDataObject;
class PickingInteractionStyle;


class SelectionHandler : public QObject
{
    Q_OBJECT

public:
    SelectionHandler();

    void setQtTableView(QTableView * tableView, QVtkTableModel * model);
    void setVtkInteractionStyle(PickingInteractionStyle * interactionStyle);
    void setDataObject(vtkDataObject * dataObject);

private slots:
    void cellPicked(vtkDataObject * dataObject, vtkIdType cellId);
    void qtSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

private:
    void createConnections();

    void updateSelections(vtkIdType cellId, bool updateView);


private:
    QTableView * m_tableView;
    QVtkTableModel * m_tableModel;
    PickingInteractionStyle * m_interactionStyle;

    vtkIdType m_currentSelection;

    vtkSmartPointer<vtkDataObject> m_dataObject;
};
