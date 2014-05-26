#pragma once

#include <QDockWidget>


class Ui_TableWidget;
class QVtkTableModel;
class QTableView;

class vtkDataSet;

class TableWidget : public QDockWidget
{
public:
    TableWidget(QWidget * parent = nullptr);
    ~TableWidget() override;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);
    QTableView * tableView();

    void showData(vtkDataSet * data);

protected:
    Ui_TableWidget * m_ui;
    QVtkTableModel * m_model;
};
