#pragma once

#include <QScopedPointer>

#include <vtkType.h>

#include <gui/data_view/AbstractDataView.h>


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
    TableView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~TableView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);

    void showDataObject(DataObject * dataObject);
    DataObject * dataObject();

signals:
    void itemDoubleClicked(DataObject * dataObject, vtkIdType index, IndexType indexType);

protected:
    QWidget * contentWidget() override;
    void selectionChangedEvent(DataObject * dataObject, vtkIdTypeArray * selection, IndexType indexType) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    QScopedPointer<Ui_TableView> m_ui;
    DataObject * m_dataObject;
    QMenu * m_selectColumnsMenu;

    QMetaObject::Connection m_hightlightUpdateConnection;
};
