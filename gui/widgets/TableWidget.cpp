#include "TableWidget.h"
#include "ui_TableWidget.h"

#include <cassert>

#include <core/Input.h>
#include <core/QVtkTableModel.h>
#include <core/data_objects/DataObject.h>



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

void TableWidget::showInput(DataObject * dataObject)
{
    if (m_dataObject == dataObject)
        return;

    assert(dataObject);
    m_dataObject = dataObject;
    view()->setModel(m_dataObject->tableModel());

    setWindowTitle("Table: " + QString::fromStdString(m_dataObject->input()->name));

    m_ui->tableView->resizeColumnsToContents();
}

DataObject * TableWidget::dataObject()
{
    return m_dataObject;
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
