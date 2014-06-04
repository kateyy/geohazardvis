#include "TableWidget.h"
#include "ui_TableWidget.h"

#include <cassert>

#include "core/Input.h"
#include "core/DataObject.h"

#include "QVtkTableModel.h"


TableWidget::TableWidget(int index, QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_TableWidget())
, m_index(index)
{
    m_ui->setupUi(this);

    //QAbstractItemModel * oldModel = m_ui->tableView->model();
    //m_ui->tableView->setModel(m_model);
    //delete oldModel;
}

TableWidget::~TableWidget()
{
    delete m_ui;
}

int TableWidget::index() const
{
    return m_index;
}

void TableWidget::showInput(std::shared_ptr<DataObject> representation)
{
    m_inputRepresentation = representation;
    view()->setModel(m_inputRepresentation->tableModel());

    setWindowTitle("Table: " + QString::fromStdString(representation->input()->name));

    m_ui->tableView->resizeColumnsToContents();
}

std::shared_ptr<DataObject> TableWidget::input()
{
    return m_inputRepresentation;
}

QVtkTableModel * TableWidget::model()
{
    assert(dynamic_cast<QVtkTableModel*>(m_ui->tableView->model()));
    return static_cast<QVtkTableModel*>(m_ui->tableView->model());
}

QTableView * TableWidget::view()
{
    assert(m_ui->tableView);
    return m_ui->tableView;
}

void TableWidget::closeEvent(QCloseEvent * event)
{
    emit closed();

    QDockWidget::closeEvent(event);
}
