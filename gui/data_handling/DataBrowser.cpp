/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include <gui/data_handling/CoordinateSystemAdjustmentWidget.h>
#include <gui/data_handling/DataBrowserTableModel.h>
#include <gui/data_view/ResidualVerificationView.h>


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
    setSelectedData(QList<DataObject *>{ dataObject });
}

void DataBrowser::setSelectedData(const QList<DataObject *> & dataObjects)
{
    QList<int> rows;
    for (auto && object : dataObjects)
    {
        rows.push_back(m_tableModel->rowForDataObject(object));
    }
    const auto currentSelection = m_ui->dataTableView->selectionModel()->selectedRows();
    if (rows.size() == currentSelection.size())
    {
        bool allEqual = true;
        for (int i = 0; allEqual && i < rows.size(); ++i)
        {
            allEqual = rows[i] == currentSelection[i].row();
        }
        if (allEqual)
        {
            return;
        }
    }

    m_ui->dataTableView->clearSelection();
    for (auto && row : rows)
    {
        m_ui->dataTableView->selectionModel()->select(
            m_ui->dataTableView->model()->index(row, 0),
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
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
        if (transformable->coordinateSystem().isValid())
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
    {
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
    }

    assignMenu->popup(position);
}

void DataBrowser::menuOpenInResidualSubview(const QPoint & position, DataObject * dataObject,
    ResidualVerificationView * residualView)
{
    // Special handling of the residual view: the clicked data object can either be shown
    // is observation or as model data. It does, however, not make sense to pass multiple
    // objects to the view.
    // TODO handling that much specialized logic here is not really nice.
    auto handlePopupClick = [residualView, dataObject] (const bool observation)
    {
        const auto currentData = observation
            ? residualView->observationData()
            : residualView->modelData();

        auto newData = currentData != dataObject
            ? dataObject                            // show event
            : static_cast<DataObject *>(nullptr);   // hide event

        if (observation)
        {
            residualView->setObservationData(newData);
        }
        else
        {
            residualView->setModelData(newData);
        }
    };

    const QString title = "Show in Residual View";
    auto residualViewPopup = new QMenu(title, this);
    residualViewPopup->setAttribute(Qt::WA_DeleteOnClose);

    auto titleA = residualViewPopup->addAction(title);
    titleA->setEnabled(false);
    auto titleFont = titleA->font();
    titleFont.setBold(true);
    titleA->setFont(titleFont);

    residualViewPopup->addSeparator();
    auto observationAction = residualViewPopup->addAction("as Observation Data");
    auto modelAction = residualViewPopup->addAction("as Model Data");
    connect(observationAction, &QAction::triggered, std::bind(handlePopupClick, true));
    connect(modelAction, &QAction::triggered, std::bind(handlePopupClick, false));

    residualViewPopup->popup(position);
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
        showTable();
        return;
    case 1:
    {
        auto dataObject = m_tableModel->dataObjectAt(index);
        if (!dataObject)
        {
            return;
        }

        if (auto residualView = dynamic_cast<ResidualVerificationView *>(
            m_dataMapping->focusedRenderView()))
        {
            menuOpenInResidualSubview(position, dataObject, residualView);
            return;
        }

        if (dataObject->dataSet())
        {
            changeRenderedVisibility(dataObject);
            return;
        }
        menuAssignDataToIndexes(position, dataObject);
        return;
    }
    case 2:
        removeFile();
        return;
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
