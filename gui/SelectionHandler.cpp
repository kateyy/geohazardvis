#include "SelectionHandler.h"

#include <cassert>

#include <QTableView>
#include <QAction>
#include <QMenu>
#include <QDebug>

#include <core/data_objects/DataObject.h>

#include "widgets/TableWidget.h"
#include "widgets/RenderWidget.h"
#include "IPickingInteractorStyle.h"


SelectionHandler & SelectionHandler::instance()
{
    static SelectionHandler selectionHandler;
    return selectionHandler;
}

SelectionHandler::SelectionHandler()
    : m_syncToggleMenu(nullptr)
{
}

SelectionHandler::~SelectionHandler()
{
    // delete QActions associated with the views
    qDeleteAll(m_tableWidgets.values());
    qDeleteAll(m_renderWidgets.values());
}

void SelectionHandler::addTableView(TableWidget * tableWidget)
{
    QAction * syncToggleAction = new QAction(tableWidget->windowTitle(), this);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);
    m_tableWidgets.insert(tableWidget, syncToggleAction);
    connect(tableWidget, &QWidget::windowTitleChanged, syncToggleAction, &QAction::setText);
    connect(tableWidget, &TableWidget::cellSelected, this, &SelectionHandler::syncRenderViewsWithTable);
    connect(tableWidget, &TableWidget::cellDoubleClicked, 
        [this](DataObject * dataObject, int row) {
        qDebug() << sender();
        renderViewsLookAt(dataObject, static_cast<vtkIdType>(row));
    });

    updateSyncToggleMenu();
}

void SelectionHandler::addRenderView(RenderWidget * renderWidget)
{
    QAction * syncToggleAction = new QAction(renderWidget->windowTitle(), this);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);
    m_renderWidgets.insert(renderWidget, syncToggleAction);
    m_actionForInteractor.insert(renderWidget->interactorStyle(), syncToggleAction);
    connect(renderWidget, &QWidget::windowTitleChanged, syncToggleAction, &QAction::setText);
    connect(renderWidget->interactorStyle(), &IPickingInteractorStyle::cellPicked, this, &SelectionHandler::syncRenderAndTableViews);

    updateSyncToggleMenu();
}

void SelectionHandler::removeTableView(TableWidget * tableWidget)
{
    QAction * action = m_tableWidgets[tableWidget];

    m_tableWidgets.remove(tableWidget);

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::removeRenderView(RenderWidget * renderWidget)
{
    QAction * action = m_renderWidgets[renderWidget];

    m_renderWidgets.remove(renderWidget);
    m_actionForInteractor.remove(renderWidget->interactorStyle());

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::setSyncToggleMenu(QMenu * syncToggleMenu)
{
    m_syncToggleMenu = syncToggleMenu;
}

void SelectionHandler::syncRenderViewsWithTable(DataObject * dataObject, vtkIdType cellId)
{
    QAction * action = m_tableWidgets.value(static_cast<TableWidget*>(sender()), nullptr);
    assert(action);
    if (!action->isChecked())
        return;

    for (auto it = m_renderWidgets.begin(); it != m_renderWidgets.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObjects().contains(dataObject))
            it.key()->interactorStyle()->highlightCell(cellId, dataObject);
    }
}

void SelectionHandler::syncRenderAndTableViews(DataObject * dataObject, vtkIdType cellId)
{
    QAction * action = m_actionForInteractor.value(static_cast<IPickingInteractorStyle*>(sender()), nullptr);
    assert(action);
    if (!action->isChecked())
        return;

    for (auto it = m_renderWidgets.begin(); it != m_renderWidgets.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObjects().contains(dataObject))
            it.key()->interactorStyle()->highlightCell(cellId, dataObject);
    }
    for (auto it = m_tableWidgets.begin(); it != m_tableWidgets.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObject() == dataObject)
            it.key()->selectCell(cellId);
    }
}

void SelectionHandler::renderViewsLookAt(DataObject * dataObject, vtkIdType cellId)
{
    for (auto it = m_renderWidgets.begin(); it != m_renderWidgets.end(); ++it)
    {
        if (it.value()->isChecked())
            it.key()->interactorStyle()->lookAtCell(dataObject, cellId);
    }
}

void SelectionHandler::updateSyncToggleMenu()
{
    m_syncToggleMenu->clear();

    m_syncToggleMenu->setEnabled(!m_tableWidgets.empty() || !m_renderWidgets.empty());


    m_syncToggleMenu->addActions(m_tableWidgets.values());
    m_syncToggleMenu->addSeparator();
    m_syncToggleMenu->addActions(m_renderWidgets.values());
}
