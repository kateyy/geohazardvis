#include "DataBrowser.h"
#include "ui_DataBrowser.h"

#include <QMessageBox>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/widgets/RenderWidget.h>
#include "DataBrowserTableModel.h"


DataBrowser::DataBrowser(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui(new Ui_DataBrowser)
    , m_tableModel(new DataBrowserTableModel)
{
    m_ui->setupUi(this);

    m_tableModel->setParent(m_ui->dataTableView);
    auto blub = m_ui->dataTableView->selectionModel();
    m_ui->dataTableView->setModel(m_tableModel);

    connect(m_ui->dataTableView, &QAbstractItemView::clicked, this, &DataBrowser::evaluateItemViewClick);
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

void DataBrowser::showTable()
{
    for (DataObject * dataObject : selectedDataObjects())
        m_dataMapping->openInTable(dataObject);
}

void DataBrowser::openRenderView()
{
    QList<DataObject *> selection = selectedDataObjects();
    if (selection.isEmpty())
        return;

    QString dataType = selection.first()->dataTypeName();
    QStringList invalidObjects;
    QList<DataObject *> oneTypeObjects;

    for (DataObject * dataObject : selectedDataObjects())
    {
        if (dataObject->dataTypeName() == dataType)
            oneTypeObjects << dataObject;
        else
            invalidObjects << dataObject->name();
    }

    if (!invalidObjects.isEmpty())
        QMessageBox::warning(this, "Invalid data selection", QString("Cannot render 2D and 3D data in the same render view!")
            + QString("\nDiscarded objects:\n") + invalidObjects.join('\n'));

    m_dataMapping->openInRenderView(oneTypeObjects);
}

void DataBrowser::changeRenderedVisibility(DataObject * clickedObject)
{
    QList<DataObject *> selection = selectedDataObjects();
    if (selection.isEmpty())
        return;

    RenderWidget * renderView = m_dataMapping->focusedRenderView();
    // no current render view: add selection to new view
    if (!renderView)
    {
        m_dataMapping->openInRenderView(selection);
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

void DataBrowser::setupGuiFor(RenderWidget * renderView)
{
    QSet<const DataObject *> allObjects;
    for (const DataObject * dataObject : m_dataObjects)
        allObjects << dataObject;

    for (const RenderedData * renderedData : renderView->renderedData())
    {
        allObjects.remove(renderedData->dataObject());

        m_tableModel->setVisibility(renderedData->dataObject(), renderedData->isVisible());
    }
}

QList<DataObject *> DataBrowser::selectedDataObjects() const
{
    QModelIndexList selection = m_ui->dataTableView->selectionModel()->selectedRows();

    QList<DataObject *> selectedObjects;

    for (const QModelIndex & index : selection)
        selectedObjects << m_tableModel->dataObjectAt(index.row());

    return selectedObjects;
}
