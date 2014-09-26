#include "TableView.h"
#include "ui_TableView.h"

#include <cassert>

#include <QMouseEvent>

#include <core/table_model/QVtkTableModel.h>
#include <core/data_objects/DataObject.h>

#include <gui/SelectionHandler.h>


TableView::TableView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_ui(new Ui_TableView())
    , m_dataObject(nullptr)
{
    m_ui->setupUi(this);
    m_ui->tableView->viewport()->installEventFilter(this);
    
    SelectionHandler::instance().addTableView(this);
}

TableView::~TableView()
{
    SelectionHandler::instance().removeTableView(this);

    delete m_ui;
}

bool TableView::isTable() const
{
    return true;
}

bool TableView::isRenderer() const
{
    return false;
}

QString TableView::friendlyName() const
{
    return QString::number(index()) + ": " + m_dataObject->name();
}

void TableView::showDataObject(DataObject * dataObject)
{
    if (m_dataObject == dataObject)
        return;

    assert(dataObject);
    m_dataObject = dataObject;
    setModel(m_dataObject->tableModel());

    updateTitle();

    m_ui->tableView->resizeColumnsToContents();
}

DataObject * TableView::dataObject()
{
    return m_dataObject;
}

void TableView::selectCell(int cellId)
{
    m_ui->tableView->selectRow(cellId);
}

QVtkTableModel * TableView::model()
{
    assert(dynamic_cast<QVtkTableModel*>(m_ui->tableView->model()));
    return static_cast<QVtkTableModel*>(m_ui->tableView->model());
}

void TableView::setModel(QVtkTableModel * model)
{
    if (QVtkTableModel * oldModel = static_cast<QVtkTableModel *>(m_ui->tableView->model()))
        disconnect(oldModel, &QVtkTableModel::dataModified, this, &TableView::dataModified);

    m_ui->tableView->setModel(model);
    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TableView::emitCellSelected);
    connect(model, &QVtkTableModel::dataModified, this, &TableView::dataModified);
}

QWidget * TableView::contentWidget()
{
    return m_ui->tableView;
}

bool TableView::eventFilter(QObject * obj, QEvent * ev)
{
    if (ev->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent * event = static_cast<QMouseEvent*>(ev);
        QModelIndex index = m_ui->tableView->indexAt(event->pos());
        // cell index column. all other columns may be editable
        if (index.column() == 0)
        {
            emit cellDoubleClicked(m_dataObject, index.row());
            return true;
        }
    }

    return AbstractDataView::eventFilter(obj, ev);
}

void TableView::emitCellSelected(const QItemSelection & selected, const QItemSelection & /*deselected*/)
{
    if (selected.indexes().isEmpty())
    {
        emit cellSelected(m_dataObject, vtkIdType(-1));
        return;
    }

    emit cellSelected(m_dataObject, vtkIdType(selected.indexes().first().row()));
}
