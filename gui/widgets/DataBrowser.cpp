#include "DataBrowser.h"
#include "ui_DataBrowser.h"

#include <QMouseEvent>
#include <QMenu>

#include <vtkFloatArray.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RawVectorData.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/widgets/DataBrowserTableModel.h>


DataBrowser::DataBrowser(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui(new Ui_DataBrowser)
    , m_tableModel(new DataBrowserTableModel)
    , m_dataMapping(nullptr)
{
    m_ui->setupUi(this);

    m_tableModel->setParent(m_ui->dataTableView);
    m_ui->dataTableView->setModel(m_tableModel);

    m_ui->dataTableView->viewport()->installEventFilter(this);

    connect(&DataSetHandler::instance(), &DataSetHandler::dataObjectsChanged, this, &DataBrowser::updateModelForFocusedView);
    connect(&DataSetHandler::instance(), &DataSetHandler::rawVectorsChanged, this, &DataBrowser::updateModelForFocusedView);

    connect(m_ui->dataTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
        [this] (const QItemSelection &selected, const QItemSelection &) {
        if (selected.indexes().isEmpty())
            return;
        
        emit selectedDataChanged(m_tableModel->dataObjectAt(selected.indexes().first()));
    });
}

DataBrowser::~DataBrowser()
{
    delete m_ui;
}

void DataBrowser::setDataMapping(DataMapping * dataMapping)
{
    if (m_dataMapping)
        disconnect(m_dataMapping, &DataMapping::focusedRenderViewChanged, this, &DataBrowser::updateModel);

    m_dataMapping = dataMapping;
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, this, &DataBrowser::updateModel);
}

void DataBrowser::setSelectedData(DataObject * data)
{
    m_ui->dataTableView->clearSelection();
    m_ui->dataTableView->selectRow(m_tableModel->rowForDataObject(data));
}

bool DataBrowser::eventFilter(QObject * /*obj*/, QEvent * ev)
{
    QModelIndex index;

    QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(ev);

    switch (ev->type())
    {
    case QEvent::MouseButtonPress:
        index = m_ui->dataTableView->indexAt(mouseEvent->pos());
        break;
    case QEvent::MouseButtonRelease:
        index = m_ui->dataTableView->indexAt(mouseEvent->pos());
        evaluateItemViewClick(index, mouseEvent->globalPos());
        break;
    default:
        return false;
    }

    // for multi selections: don't discard the selection when clicking on the buttons
    QItemSelection selection = m_ui->dataTableView->selectionModel()->selection();
    if ((selection.size() > 1)
        && selection.contains(index)
        && (index.column() < m_tableModel->numButtonColumns()))
        return true;

    return false;
}

void DataBrowser::updateModelForFocusedView()
{
    updateModel(m_dataMapping->focusedRenderView());
}

void DataBrowser::updateModel(AbstractRenderView * renderView)
{
    QList<DataObject *> visibleObjects;
    if (renderView)
        visibleObjects = renderView->dataObjects();

    m_tableModel->updateDataList(visibleObjects);

    m_ui->dataTableView->resizeColumnsToContents();
}

void DataBrowser::showTable()
{
    for (DataObject * dataObject : selectedDataObjects())
        m_dataMapping->openInTable(dataObject);
}

void DataBrowser::changeRenderedVisibility(DataObject * clickedObject)
{
    QList<DataObject *> selection = selectedDataSets();
    if (selection.isEmpty())
        return;

    AbstractRenderView * renderView = m_dataMapping->focusedRenderView();
    // no current render view: add selection to new view
    if (!renderView)
    {
        renderView = m_dataMapping->openInRenderView(selection);
        updateModel(renderView);
        return;
    }

    bool wasVisible = renderView->contains(clickedObject);

    // if multiple objects are selected: first change all to the same (current) value
    // and toggle visibility only with the next click
    bool setToVisible = !wasVisible;
    if (selection.size() > 1)
    {
        bool allSame = true;
        bool firstValue = renderView->contains(selection.first());
        for (DataObject * dataObject : selection)
            allSame = allSame && (firstValue == renderView->contains(dataObject));

        if (!allSame)
            setToVisible = wasVisible;
    }

    if (setToVisible)
        // dataMapping may open multiple views if needed
        m_dataMapping->addToRenderView(selection, renderView);
    else
        renderView->hideDataObjects(selection);

    updateModel(renderView);
}

void DataBrowser::menuAssignDataToIndexes(const QPoint & position, DataObject * clickedData)
{
    QString title = "assign to indexes";

    RawVectorData * rawVector = dynamic_cast<RawVectorData*>(clickedData);
    if (!rawVector)
        return;

    vtkDataArray * dataArray = rawVector->dataArray();

    QMenu * assignMenu = new QMenu(title, this);
    assignMenu->setAttribute(Qt::WA_DeleteOnClose);

    QAction * titleA = assignMenu->addAction(title);
    titleA->setEnabled(false);
    QFont titleFont = titleA->font();
    titleFont.setBold(true);
    titleA->setFont(titleFont);

    assignMenu->addSeparator();
    
    if (DataSetHandler::instance().dataSets().isEmpty())
    {
        QAction * loadFirst = assignMenu->addAction("(load geometries/images first)");
        loadFirst->setEnabled(false);
    }
    else
        for (DataObject * indexes : DataSetHandler::instance().dataSets())
        {
        QAction * assignAction = assignMenu->addAction(indexes->name());
        connect(assignAction, &QAction::triggered,
            [indexes, dataArray] (bool) {
            indexes->addDataArray(dataArray);
        });
        }

    assignMenu->popup(position);
}

void DataBrowser::removeFile()
{
    QList<DataObject *> selection = selectedDataObjects();

    m_dataMapping->removeDataObjects(selection);
    DataSetHandler::instance().deleteData(selection);

    updateModelForFocusedView();
}

void DataBrowser::evaluateItemViewClick(const QModelIndex & index, const QPoint & position)
{
    switch (index.column())
    {
    case 0:
        return showTable();
    case 1:
    {
        DataObject * dataObject = m_tableModel->dataObjectAt(index);
        if (dataObject && dataObject->dataSet())
            return changeRenderedVisibility(dataObject);
        if (dataObject /* && !dataObject->dataSet() */)
            return menuAssignDataToIndexes(position, dataObject);
    }
    case 2:
        return removeFile();
    }
}

QList<DataObject *> DataBrowser::selectedDataObjects() const
{
    QModelIndexList selection = m_ui->dataTableView->selectionModel()->selectedRows();

    return m_tableModel->dataObjects(selection);
}

QList<DataObject *> DataBrowser::selectedDataSets() const
{
    QModelIndexList selection = m_ui->dataTableView->selectionModel()->selectedRows();

    return m_tableModel->dataSets(selection);
}

QList<DataObject *> DataBrowser::selectedAttributeVectors() const
{
    QModelIndexList selection = m_ui->dataTableView->selectionModel()->selectedRows();

    return m_tableModel->rawVectors(selection);
}
