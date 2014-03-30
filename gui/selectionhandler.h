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

private:
    void createConnections();

    void qtSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);


private:
    QTableView * m_tableView;
    QVtkTableModel * m_tableModel;
    PickingInteractionStyle * m_interactionStyle;

    vtkSmartPointer<vtkDataObject> m_dataObject;
};
