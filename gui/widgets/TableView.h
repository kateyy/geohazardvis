#pragma once

#include <QDockWidget>

#include <vtkType.h>


class QTableView;
class QItemSelection;

class Ui_TableView;
class QVtkTableModel;
class DataObject;


class TableView : public QDockWidget
{
    Q_OBJECT

public:
    TableView(int index, QWidget * parent = nullptr);
    ~TableView() override;

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
    /** signaled when the widget receive the keyboard focus (focusInEvent) */
    void focused(TableView * tableView);

protected:
    void focusInEvent(QFocusEvent * event);
    void focusOutEvent(QFocusEvent * event);

    bool eventFilter(QObject * obj, QEvent * ev) override;

private slots:
    void emitCellSelected(const QItemSelection & selected, const QItemSelection & deselected);
    
private:
    void closeEvent(QCloseEvent * event) override;

private:
    const int m_index;
    Ui_TableView * m_ui;
    DataObject * m_dataObject;
};
