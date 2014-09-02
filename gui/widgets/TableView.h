#pragma once

#include "AbstractDataView.h"

#include <vtkType.h>


class QTableView;
class QItemSelection;

class Ui_TableView;
class QVtkTableModel;
class DataObject;


class TableView : public AbstractDataView
{
    Q_OBJECT

public:
    TableView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~TableView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);

    void showDataObject(DataObject * dataObject);
    DataObject * dataObject();

public slots:
    void selectCell(int cellId);

signals:
    void cellSelected(DataObject * dataObject, vtkIdType cellId);
    void cellDoubleClicked(DataObject * dataObject, vtkIdType cellId);

protected:
    QWidget * contentWidget() override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

private slots:
    void emitCellSelected(const QItemSelection & selected, const QItemSelection & deselected);

private:
    Ui_TableView * m_ui;
    DataObject * m_dataObject;
};
