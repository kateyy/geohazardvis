#include "TableView.h"
#include "ui_TableView.h"

#include <cassert>

#include <QMouseEvent>
#include <QMenu>
#include <QAction>

#include <vtkIdTypeArray.h>

#include <core/table_model/QVtkTableModel.h>
#include <core/data_objects/DataObject.h>


TableView::TableView(DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(dataMapping, index, parent, flags)
    , m_ui{ std::make_unique<Ui_TableView>() }
    , m_dataObject{ nullptr }
    , m_selectColumnsMenu{ nullptr }
{
    m_ui->setupUi(this);
    m_selectColumnsMenu = new QMenu(m_ui->tableView);

    m_ui->tableView->viewport()->installEventFilter(this);

    m_ui->tableView->verticalHeader()->setFixedWidth(15);

    m_ui->tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui->tableView->horizontalHeader(), &QHeaderView::customContextMenuRequested, [this] (const QPoint & position) {
        m_selectColumnsMenu->popup(m_ui->tableView->horizontalHeader()->mapToGlobal(position));
    });
}

TableView::~TableView() = default;

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

void TableView::showDataObject(DataObject & dataObject)
{
    if (m_dataObject == &dataObject)
    {
        return;
    }
    m_dataObject = &dataObject;

    setModel(m_dataObject->tableModel());

    updateTitle();

    QTableView * view = m_ui->tableView;
    view->resizeColumnsToContents();

    m_selectColumnsMenu->clear();
    for (int i = 0; i < m_dataObject->tableModel()->columnCount(); ++i)
    {
        const auto title = m_dataObject->tableModel()->headerData(i, Qt::Horizontal).toString();
        auto action = m_selectColumnsMenu->addAction(title);
        action->setCheckable(true);
        action->setChecked(!view->isColumnHidden(i));
        connect(action, &QAction::triggered, [view, i] (bool checked)
        {
            if (checked)
            {
                view->showColumn(i);
            }
            else
            {
                view->hideColumn(i);
            }
        });
    }
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
        {
            return;
        }

        vtkIdType index = model->itemIdAt(selected.indexes().first());
        model->setHighlightItemId(index);

        emit objectPicked(m_dataObject, index, model->indexType());
    });
}

QWidget * TableView::contentWidget()
{
    return m_ui->tableView;
}

void TableView::selectionChangedEvent(DataObject * dataObject, vtkIdTypeArray * indices, IndexType indexType)
{
    assert((dataObject != nullptr) == (indices != nullptr));

    if (dataObject && dataObject != m_dataObject)
    {
        return;
    }

    if (!dataObject || (indices->GetSize() == 0) || (indexType != model()->indexType()))
    {
        model()->setHighlightItemId(-1);
        return;
    }

    // TODO implement multiple selections

    auto index = indices->GetValue(0);

    QModelIndex selection(model()->index(static_cast<int>(index), 0));
    m_ui->tableView->scrollTo(selection);

    model()->setHighlightItemId(index);
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
            emit itemDoubleClicked(m_dataObject, id, model()->indexType());
            return true;
        }
    }

    return AbstractDataView::eventFilter(obj, ev);
}
