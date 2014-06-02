#pragma once

#include <memory>

#include <QDockWidget>


class QTableView;

class vtkDataSet;

class Ui_TableWidget;
class QVtkTableModel;
class InputRepresentation;


class TableWidget : public QDockWidget
{
    Q_OBJECT

public:
    TableWidget(int index, QWidget * parent = nullptr);
    ~TableWidget() override;

    int index() const;

    QVtkTableModel * model();
    QTableView * view();

    void showInput(std::shared_ptr<InputRepresentation> representation);
    std::shared_ptr<InputRepresentation> input();

signals:
    void closed();

private:
    void closeEvent(QCloseEvent * event) override;

private:
    const int m_index;
    Ui_TableWidget * m_ui;
    std::shared_ptr<InputRepresentation> m_inputRepresentation;
};
