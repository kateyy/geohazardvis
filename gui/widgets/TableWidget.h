#pragma once

#include <memory>

#include <QDockWidget>


class QTableView;

class vtkDataSet;

class Ui_TableWidget;
class QVtkTableModel;
class Property;


class TableWidget : public QDockWidget
{
    Q_OBJECT

public:
    TableWidget(int index, QWidget * parent = nullptr);
    ~TableWidget() override;

    int index() const;

    QVtkTableModel * model();
    QTableView * view();

    void showInput(std::shared_ptr<Property> representation);
    std::shared_ptr<Property> input();

signals:
    void closed();

private:
    void closeEvent(QCloseEvent * event) override;

private:
    const int m_index;
    Ui_TableWidget * m_ui;
    std::shared_ptr<Property> m_inputRepresentation;
};
