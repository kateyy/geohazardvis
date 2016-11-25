#pragma once

#include <memory>

#include <gui/data_view/AbstractDataView.h>


class QTableView;
class QItemSelection;
class QMenu;

class Ui_TableView;
class QVtkTableModel;


class GUI_API TableView : public AbstractDataView
{
    Q_OBJECT

public:
    TableView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~TableView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QVtkTableModel * model();
    void setModel(QVtkTableModel * model);

    void showDataObject(DataObject & dataObject);
    DataObject * dataObject();

signals:
    void itemDoubleClicked(const DataSelection & selection);

protected:
    QWidget * contentWidget() override;
    void onSetSelection(const DataSelection & selection) override;
    void onClearSelection() override;

    std::pair<QString, std::vector<QString>> friendlyNameInternal() const override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    std::unique_ptr<Ui_TableView> m_ui;
    DataObject * m_dataObject;
    QMenu * m_selectColumnsMenu;

    QMetaObject::Connection m_hightlightUpdateConnection;

private:
    Q_DISABLE_COPY(TableView)
};
