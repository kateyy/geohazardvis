#include "TableWidget.h"
#include "ui_TableWidget.h"

#include <cassert>

#include <core/Input.h>
#include <core/QVtkTableModel.h>
#include <core/data_objects/DataObject.h>

#include <gui/SelectionHandler.h>


TableWidget::TableWidget(int index, QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_TableWidget())
    , m_index(index)
{
    m_ui->setupUi(this);
    
    SelectionHandler::instance().addTableView(this);
}

TableWidget::~TableWidget()
{
    SelectionHandler::instance().removeTableView(this);

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
    setModel(m_dataObject->tableModel());

    setWindowTitle("Table: " + QString::fromStdString(m_dataObject->input()->name));

    m_ui->tableView->resizeColumnsToContents();
}

DataObject * TableWidget::dataObject()
{
    return m_dataObject;
}

void TableWidget::selectCell(int cellId)
{
    m_ui->tableView->selectRow(cellId);
}

QVtkTableModel * TableWidget::model()
{
    assert(dynamic_cast<QVtkTableModel*>(m_ui->tableView->model()));
    return static_cast<QVtkTableModel*>(m_ui->tableView->model());
}

void TableWidget::setModel(QVtkTableModel * model)
{
    m_ui->tableView->setModel(model);
    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TableWidget::selectionChanged);
}

void TableWidget::selectionChanged(const QItemSelection & selected, const QItemSelection & /*deselected*/)
{
    if (selected.indexes().isEmpty())
    {
        emit cellSelected(-1);
    }

    emit cellSelected(selected.indexes().first().row());
}

void TableWidget::closeEvent(QCloseEvent * event)
{
    emit closed();

    QDockWidget::closeEvent(event);
}
