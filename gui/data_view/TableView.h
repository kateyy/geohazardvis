#pragma once

#include <gui/data_view/AbstractDataView.h>

#include <vtkType.h>


class QTableView;
class QItemSelection;
class QMenu;

class Ui_TableView;
class QVtkTableModel;
class DataObject;


class GUI_API TableView : public AbstractDataView
{
    Q_OBJECT

public:
    TableView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~TableView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);

    void showDataObject(DataObject * dataObject);
    DataObject * dataObject();

signals:
    void itemDoubleClicked(DataObject * dataObject, vtkIdType itemId);

protected:
    QWidget * contentWidget() override;
    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    Ui_TableView * m_ui;
    DataObject * m_dataObject;
    QMenu * m_selectColumnsMenu;

    QMetaObject::Connection m_hightlightUpdateConnection;
};
