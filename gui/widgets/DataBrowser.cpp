#include "DataBrowser.h"
#include "ui_DataBrowser.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QDebug>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/data_view/RenderView.h>
#include "DataBrowserTableModel.h"


DataBrowser::DataBrowser(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui(new Ui_DataBrowser)
    , m_tableModel(new DataBrowserTableModel)
{
    m_ui->setupUi(this);

    m_tableModel->setParent(m_ui->dataTableView);
    m_ui->dataTableView->setModel(m_tableModel);

    m_ui->dataTableView->viewport()->installEventFilter(this);
}

DataBrowser::~DataBrowser()
{
    delete m_ui;

    qDeleteAll(m_dataObjects);
}

void DataBrowser::setDataMapping(DataMapping * dataMapping)
{
    if (m_dataMapping)
        disconnect(m_dataMapping, &DataMapping::focusedRenderViewChanged, this, &DataBrowser::setupGuiFor);

    m_dataMapping = dataMapping;
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, this, &DataBrowser::setupGuiFor);
}

void DataBrowser::addDataObject(DataObject * dataObject)
{
    m_dataObjects << dataObject;

    m_tableModel->addDataObject(dataObject);

    m_ui->dataTableView->resizeColumnsToContents();
}

bool DataBrowser::eventFilter(QObject * /*obj*/, QEvent * ev)
{
    QModelIndex index;

    switch (ev->type())
    {
    case QEvent::MouseButtonPress:
        index = m_ui->dataTableView->indexAt(static_cast<QMouseEvent *>(ev)->pos());
        break;
    case QEvent::MouseButtonRelease:
        index = m_ui->dataTableView->indexAt(static_cast<QMouseEvent *>(ev)->pos());
        evaluateItemViewClick(index);
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

void DataBrowser::showTable()
{
    for (DataObject * dataObject : selectedDataObjects())
        m_dataMapping->openInTable(dataObject);
}

void DataBrowser::changeRenderedVisibility(DataObject * clickedObject)
{
    QList<DataObject *> selection = selectedDataObjects();
    if (selection.isEmpty())
        return;

    RenderView * renderView = m_dataMapping->focusedRenderView();
    // no current render view: add selection to new view
    if (!renderView)
    {
        renderView = m_dataMapping->openInRenderView(selection);
        renderView->setFocus();
        setupGuiFor(renderView);
        return;
    }

    bool wasVisible = renderView->isVisible(clickedObject);

    // if multiple objects are selected: first change all to the same (current) value
    // and toggle visibility only with the next click
    bool setToVisible = !wasVisible;
    if (selection.size() > 1)
    {
        bool allSame = true;
        bool firstValue = renderView->isVisible(selection.first());
        for (DataObject * dataObject : selection)
            allSame = allSame && (firstValue == renderView->isVisible(dataObject));

        if (!allSame)
            setToVisible = wasVisible;
    }

    if (setToVisible)
        renderView->addDataObjects(selectedDataObjects());
    else
        renderView->hideDataObjects(selectedDataObjects());

    setupGuiFor(renderView);
}

void DataBrowser::removeFile()
{
    QList<DataObject *> selection = selectedDataObjects();

    m_dataMapping->removeDataObjects(selection);

    for (DataObject * dataObject : selection)
    {
        m_tableModel->removeDataObject(dataObject);

        m_dataObjects.removeOne(dataObject);
    }

    qDeleteAll(selection);
}

void DataBrowser::evaluateItemViewClick(const QModelIndex & index)
{
    switch (index.column())
    {
    case 0:
        return showTable();
    case 1:
        return changeRenderedVisibility(m_tableModel->dataObjectAt(index));
    case 2:
        return removeFile();
    }
}

void DataBrowser::setupGuiFor(RenderView * renderView)
{
    QSet<const DataObject *> allObjects;
    for (const DataObject * dataObject : m_dataObjects)
        allObjects << dataObject;

    if (!renderView)
    {
        m_tableModel->setNoRendererFocused();
        return;
    }

    for (const RenderedData * renderedData : renderView->renderedData())
    {
        allObjects.remove(renderedData->dataObject());

        m_tableModel->setVisibility(renderedData->dataObject(), renderedData->isVisible());
    }

    // hide all not rendered objects
    for (const DataObject * dataObject : allObjects)
        m_tableModel->setVisibility(dataObject, false);
}

QList<DataObject *> DataBrowser::selectedDataObjects() const
{
    QModelIndexList selection = m_ui->dataTableView->selectionModel()->selectedRows();

    QList<DataObject *> selectedObjects;

    for (const QModelIndex & index : selection)
        selectedObjects << m_tableModel->dataObjectAt(index.row());

    return selectedObjects;
}
