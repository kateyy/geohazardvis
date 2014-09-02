#include "DataBrowser.h"
#include "ui_DataBrowser.h"

#include <QMessageBox>

#include <core/data_objects/DataObject.h>

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

void DataBrowser::setRenderedVisibility(bool visible)
{
    RenderWidget * renderView = m_dataMapping->focusedRenderView();
    if (!renderView)
        return;

    if (visible)
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

void DataBrowser::setupGuiFor(RenderWidget * renderView)
{

}

QList<DataObject *> DataBrowser::selectedDataObjects() const
{
    QModelIndexList selection = m_ui->dataTableView->selectionModel()->selectedRows();

    QList<DataObject *> selectedObjects;

    for (const QModelIndex & index : selection)
        selectedObjects << m_tableModel->dataObjectAt(index.row());

    return selectedObjects;
}
