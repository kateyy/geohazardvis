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

    m_ui->tableView->verticalHeader()->setFixedWidth(15);
    
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

QVtkTableModel * TableView::model()
{
    assert(dynamic_cast<QVtkTableModel*>(m_ui->tableView->model()));
    return static_cast<QVtkTableModel*>(m_ui->tableView->model());
}

void TableView::setModel(QVtkTableModel * model)
{
    disconnect(m_hightlightUpdateConnection);

    m_ui->tableView->setModel(model);
    m_hightlightUpdateConnection = connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
        [this, model] (const QItemSelection & selected, const QItemSelection & /*deselected*/)
    {
        if (selected.indexes().isEmpty())
            return;

        vtkIdType id = model->itemIdAt(selected.indexes().first());
        model->setHighlightItemId(id);

        emit objectPicked(m_dataObject, id);
    });
}

QWidget * TableView::contentWidget()
{
    return m_ui->tableView;
}

void TableView::highlightedIdChangedEvent(DataObject * /*dataObject*/, vtkIdType itemId)
{
    QModelIndex selection(model()->index(itemId, 0));
    m_ui->tableView->scrollTo(selection);

    model()->setHighlightItemId(itemId);
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
            vtkIdType id = static_cast<vtkIdType>(index.row());
            emit itemDoubleClicked(m_dataObject, id);
            return true;
        }
    }

    return AbstractDataView::eventFilter(obj, ev);
}
