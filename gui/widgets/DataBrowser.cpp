#include "DataBrowser.h"
#include "ui_DataBrowser.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QMenu>

#include <vtkFloatArray.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/data_objects/RawVectorData.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/widgets/CoordinateSystemAdjustmentWidget.h>
#include <gui/widgets/DataBrowserTableModel.h>


DataBrowser::DataBrowser(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui{ std::make_unique<Ui_DataBrowser>() }
    , m_tableModel{ new DataBrowserTableModel }
    , m_dataMapping{ nullptr }
{
    m_ui->setupUi(this);

    m_tableModel->setParent(m_ui->dataTableView);
    m_ui->dataTableView->setModel(m_tableModel);

    m_ui->dataTableView->viewport()->installEventFilter(this);

    connect(m_ui->dataTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
        [this] (const QItemSelection &selected, const QItemSelection &) {
        if (selected.indexes().isEmpty())
        {
            return;
        }
        
        emit selectedDataChanged(m_tableModel->dataObjectAt(selected.indexes().first()));
    });
}

DataBrowser::~DataBrowser() = default;

void DataBrowser::setDataMapping(DataMapping * dataMapping)
{
    if (m_dataMapping)
    {
        disconnect(m_dataMapping, &DataMapping::focusedRenderViewChanged, this, &DataBrowser::updateModel);
        disconnect(&m_dataMapping->dataSetHandler(), &DataSetHandler::dataObjectsChanged, this, &DataBrowser::updateModelForFocusedView);
        disconnect(&m_dataMapping->dataSetHandler(), &DataSetHandler::rawVectorsChanged, this, &DataBrowser::updateModelForFocusedView);
    }

    m_dataMapping = dataMapping;
    if (m_dataMapping)
    {
        m_tableModel->setDataSetHandler(&m_dataMapping->dataSetHandler());
        connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, this, &DataBrowser::updateModel);
        connect(&m_dataMapping->dataSetHandler(), &DataSetHandler::dataObjectsChanged, this, &DataBrowser::updateModelForFocusedView);
        connect(&m_dataMapping->dataSetHandler(), &DataSetHandler::rawVectorsChanged, this, &DataBrowser::updateModelForFocusedView);
    }
    else
    {
        m_tableModel->setDataSetHandler(nullptr);
    }
}

void DataBrowser::setSelectedData(DataObject * dataObject)
{
    const int row = m_tableModel->rowForDataObject(dataObject);

    auto currentSelection = m_ui->dataTableView->selectionModel()->selectedRows();
    if (currentSelection.size() == 1 && currentSelection.first().row() == row)
    {
        return;
    }

    m_ui->dataTableView->clearSelection();
    m_ui->dataTableView->selectRow(m_tableModel->rowForDataObject(dataObject));
}

bool DataBrowser::eventFilter(QObject * /*obj*/, QEvent * ev)
{
    QModelIndex index;

    auto mouseEvent = static_cast<QMouseEvent *>(ev);

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
    return selection.size() > 1
        && selection.contains(index)
        && index.column() < m_tableModel->numButtonColumns();
}

void DataBrowser::updateModelForFocusedView()
{
    updateModel(m_dataMapping->focusedRenderView());
}

void DataBrowser::updateModel(AbstractRenderView * renderView)
{
    QList<DataObject *> visibleObjects;
    if (renderView)
    {
        visibleObjects = renderView->dataObjects();
    }

    m_tableModel->updateDataList(visibleObjects);

    m_ui->dataTableView->resizeColumnsToContents();
}

void DataBrowser::showTable()
{
    for (auto dataObject : selectedDataObjects())
    {
        m_dataMapping->openInTable(dataObject);
    }
}

void DataBrowser::changeRenderedVisibility(DataObject * clickedObject)
{
    auto selection = selectedDataSets();
    if (selection.isEmpty())
    {
        return;
    }

    // make sure that the clicked object is used to set rendering/view defaults
    if (selection.first() != clickedObject)
    {
        selection.removeOne(clickedObject);
        selection.push_front(clickedObject);
    }

    auto renderView = m_dataMapping->focusedRenderView();

    QList<CoordinateTransformableDataObject *> transformableMissingInfo;
    for (auto dataObject : selection)
    {
        // don't check again for currently rendered data
        if (renderView && renderView->contains(dataObject))
        {
            continue;
        }

        auto transformable = dynamic_cast<CoordinateTransformableDataObject *>(dataObject);
        if (!transformable)
        {
            continue;
        }
        if (transformable->coordinateSystem().isValid(false))
        {
            continue;
        }
        transformableMissingInfo << transformable;
    }

    if (!transformableMissingInfo.isEmpty())
    {
        QString names;
        for (auto transformable : transformableMissingInfo)
        {
            names += " - " + transformable->name() + "\n";
        }

        const auto answer = QMessageBox::question(nullptr, "Missing Coordinate System Information",
            "The coordinate system of the following data sets is not set:\n"
            + names
            + "This may lead to incorrect visualizations. Do you want the set the coordinate system now?",
            QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        switch (answer)
        {
        case QMessageBox::Yes:
        {
            CoordinateSystemAdjustmentWidget widget;
            for (auto transformable : transformableMissingInfo)
            {
                widget.setDataObject(transformable);
                widget.exec();
            }
            break;
        }
        case QMessageBox::No:
            break;
        default:
            return;
        }
    }

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
        for (auto dataObject : selection)
        {
            allSame = allSame && (firstValue == renderView->contains(dataObject));
        }

        if (!allSame)
        {
            setToVisible = wasVisible;
        }
    }

    if (setToVisible)
    {
        // dataMapping may open multiple views if needed
        m_dataMapping->addToRenderView(selection, renderView);
    }
    else
    {
        renderView->hideDataObjects(selection);
    }

    updateModel(renderView);
}

void DataBrowser::menuAssignDataToIndexes(const QPoint & position, DataObject * clickedData)
{
    const QString title = "Assign to Data Set";

    auto rawVector = dynamic_cast<RawVectorData*>(clickedData);
    if (!rawVector)
    {
        return;
    }

    auto dataArray = rawVector->dataArray();

    auto assignMenu = new QMenu(title, this);
    assignMenu->setAttribute(Qt::WA_DeleteOnClose);

    auto titleA = assignMenu->addAction(title);
    titleA->setEnabled(false);
    auto titleFont = titleA->font();
    titleFont.setBold(true);
    titleA->setFont(titleFont);

    assignMenu->addSeparator();
    
    if (m_dataMapping->dataSetHandler().dataSets().isEmpty())
    {
        auto loadFirst = assignMenu->addAction("(Load data sets first)");
        loadFirst->setEnabled(false);
    }
    else
        for (auto && indexes : m_dataMapping->dataSetHandler().dataSets())
        {
            auto assignAction = assignMenu->addAction(indexes->name());
            connect(assignAction, &QAction::triggered,
                [indexes, dataArray] (bool)
            {
                assert(dataArray);
                indexes->addDataArray(*dataArray);
            });
        }

    assignMenu->popup(position);
}

void DataBrowser::removeFile()
{
    const auto && selection = selectedDataObjects();
    QList<DataObject *> deletable;
    for (auto dataObject : selection)
    {
        if (m_dataMapping->dataSetHandler().ownsData(dataObject))
        {
            deletable << dataObject;
        }
    }

    m_dataMapping->removeDataObjects(deletable);
    m_dataMapping->dataSetHandler().deleteData(deletable);

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
        auto dataObject = m_tableModel->dataObjectAt(index);
        if (dataObject && dataObject->dataSet())
        {
            return changeRenderedVisibility(dataObject);
        }
        if (dataObject /* && !dataObject->dataSet() */)
        {
            return menuAssignDataToIndexes(position, dataObject);
        }
    }
    case 2:
        return removeFile();
    }
}

QList<DataObject *> DataBrowser::selectedDataObjects() const
{
    const auto selection = m_ui->dataTableView->selectionModel()->selectedRows();

    return m_tableModel->dataObjects(selection);
}

QList<DataObject *> DataBrowser::selectedDataSets() const
{
    const auto selection = m_ui->dataTableView->selectionModel()->selectedRows();

    return m_tableModel->dataSets(selection);
}

QList<DataObject *> DataBrowser::selectedAttributeVectors() const
{
    const auto selection = m_ui->dataTableView->selectionModel()->selectedRows();

    return m_tableModel->rawVectors(selection);
}
