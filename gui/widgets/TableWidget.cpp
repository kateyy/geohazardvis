#include "TableWidget.h"
#include "ui_TableWidget.h"

#include <cassert>

#include "QVtkTableModel.h"


TableWidget::TableWidget(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_TableWidget())
{
    m_ui->setupUi(this);

    m_model = new QVtkTableModel(m_ui->tableView);

    QAbstractItemModel * oldModel = m_ui->tableView->model();
    m_ui->tableView->setModel(m_model);
    delete oldModel;
}

TableWidget::~TableWidget()
{
    delete m_ui;
}

void TableWidget::showData(vtkDataSet * data)
{
    m_model->showData(data);
    m_ui->tableView->resizeColumnsToContents();
}

QVtkTableModel * TableWidget::model()
{
    assert(dynamic_cast<QVtkTableModel*>(m_ui->tableView->model()));
    return static_cast<QVtkTableModel*>(m_ui->tableView->model());
}

QTableView * TableWidget::tableView()
{
    assert(m_ui->tableView);
    return m_ui->tableView;
}
