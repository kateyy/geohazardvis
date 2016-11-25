#include "TableView.h"
#include "ui_TableView.h"

#include <cassert>

#include <QMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QAction>

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

TableView::~TableView()
{
    signalClosing();
}

bool TableView::isTable() const
{
    return true;
}

bool TableView::isRenderer() const
{
    return false;
}

void TableView::showDataObject(DataObject & dataObject)
{
    if (m_dataObject == &dataObject)
    {
        return;
    }
    m_dataObject = &dataObject;

    setModel(m_dataObject->tableModel());

    resetFriendlyName();
    updateTitle();

    QTableView * view = m_ui->tableView;
    view->resizeColumnsToContents();

    m_selectColumnsMenu->clear();

    if (!m_dataObject->tableModel())
    {
        QMessageBox::information(this, "", "Unfortunately, listing the selected data set in a table view is not implemented.");
        setSelection(DataSelection{});
        return;
    }

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

    setSelection(DataSelection(&dataObject));
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
        // TODO support multiple selections

        if (selected.indexes().isEmpty())
        {
            clearSelection();
            return;
        }

        setSelection(DataSelection(m_dataObject,
            model->indexType(),
            model->itemIdAt(selected.indexes().first())));
    });
}

QWidget * TableView::contentWidget()
{
    return m_ui->tableView;
}

void TableView::onSetSelection(const DataSelection & selection)
{
    // check for invalid/not implemented selection
    if ((selection.dataObject && selection.dataObject != m_dataObject)
        || selection.indexType != model()->indexType())
    {
        return;
    }

    // check for unchanged selection
    if (selection.dataObject && !selection.indices.empty() 
        && selection.indices.front() == model()->hightlightItemId())
    {
        return;
    }

    if (selection.isIndexListEmpty())
    {
        model()->setHighlightItemId(-1);
        return;
    }

    // TODO implement multiple selections

    const auto index = selection.indices.front();

    const QModelIndex modelIndex(model()->index(static_cast<int>(index), 0));
    m_ui->tableView->scrollTo(modelIndex);

    model()->setHighlightItemId(index);
}

void TableView::onClearSelection()
{
    model()->setHighlightItemId(-1);
}

std::pair<QString, std::vector<QString>> TableView::friendlyNameInternal() const
{
    auto newFriendlyName = QString::number(index()) + ": ";

    if (!m_dataObject)
    {
        newFriendlyName.append("(empty)");
    }
    else
    {
        newFriendlyName.append(m_dataObject->name());
    }

    return{ newFriendlyName, {} };
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
            emit itemDoubleClicked(DataSelection(m_dataObject, model()->indexType(), id));
            return true;
        }
    }

    return AbstractDataView::eventFilter(obj, ev);
}
