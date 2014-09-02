#include "TableView.h"
#include "ui_TableView.h"

#include <cassert>

#include <QMouseEvent>

#include <core/Input.h>
#include <core/QVtkTableModel.h>
#include <core/data_objects/DataObject.h>

#include <gui/SelectionHandler.h>


TableView::TableView(int index, QWidget * parent)
    : QDockWidget(parent)
    , m_index(index)
    , m_ui(new Ui_TableView())
    , m_dataObject(nullptr)
{
    m_ui->setupUi(this);
    m_ui->tableView->installEventFilter(this);
    m_ui->tableView->viewport()->installEventFilter(this);
    
    SelectionHandler::instance().addTableView(this);
}

TableView::~TableView()
{
    SelectionHandler::instance().removeTableView(this);

    delete m_ui;
}

int TableView::index() const
{
    return m_index;
}

void TableView::showInput(DataObject * dataObject)
{
    if (m_dataObject == dataObject)
        return;

    assert(dataObject);
    m_dataObject = dataObject;
    setModel(m_dataObject->tableModel());

    setWindowTitle("Table: " + QString::fromStdString(m_dataObject->input()->name));

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
    m_ui->tableView->setModel(model);
    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TableView::emitCellSelected);
}

void TableView::focusInEvent(QFocusEvent * /*event*/)
{
    auto f = font();
    f.setBold(true);
    setFont(f);

    emit focused(this);
}

void TableView::focusOutEvent(QFocusEvent * /*event*/)
{
    if (m_ui->tableView->hasFocus())
        return;

    auto f = font();
    f.setBold(false);
    setFont(f);
}

bool TableView::eventFilter(QObject * /*obj*/, QEvent * ev)
{
    switch (ev->type())
    {
    case QEvent::Type::MouseButtonDblClick:
    {
        QMouseEvent * event = static_cast<QMouseEvent*>(ev);
        vtkIdType row = m_ui->tableView->rowAt(event->pos().y());
        emit cellDoubleClicked(m_dataObject, row);
        return true;
    }
    case QEvent::FocusIn:
        setFocus();
        break;
    }

    return false;
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

void TableView::closeEvent(QCloseEvent * event)
{
    emit closed();

    QDockWidget::closeEvent(event);
}
