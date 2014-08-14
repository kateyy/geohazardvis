#pragma once

#include <QDockWidget>

#include <vtkType.h>


class QTableView;
class QItemSelection;

class Ui_TableWidget;
class QVtkTableModel;
class DataObject;


class TableWidget : public QDockWidget
{
    Q_OBJECT

public:
    TableWidget(int index, QWidget * parent = nullptr);
    ~TableWidget() override;

    int index() const;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);

    void showInput(DataObject * dataObject);
    DataObject * dataObject();

public slots:
    void selectCell(int cellId);

signals:
    void cellSelected(DataObject * dataObject, vtkIdType cellId);
    void cellDoubleClicked(DataObject * dataObject, vtkIdType cellId);

    void closed();

private slots:
    void emitCellSelected(const QItemSelection & selected, const QItemSelection & deselected);
    

private:
    void closeEvent(QCloseEvent * event) override;

private:
    const int m_index;
    Ui_TableWidget * m_ui;
    DataObject * m_dataObject;
};
