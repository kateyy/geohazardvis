#include "SelectionHandler.h"

#include <cassert>

#include <QTableView>
#include <QAction>
#include <QMenu>
#include <QDebug>

#include <core/data_objects/DataObject.h>

#include "data_view/TableView.h"
#include "data_view/RenderView.h"
#include "rendering_interaction/IPickingInteractorStyle.h"


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
    qDeleteAll(m_tableViews.values());
    qDeleteAll(m_renderViews.values());
}

void SelectionHandler::addTableView(TableView * tableView)
{
    QAction * syncToggleAction = new QAction(tableView->windowTitle(), this);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);
    m_tableViews.insert(tableView, syncToggleAction);
    connect(tableView, &QWidget::windowTitleChanged, syncToggleAction, &QAction::setText);
    connect(tableView, &TableView::cellSelected, this, &SelectionHandler::syncRenderViewsWithTable);
    connect(tableView, &TableView::cellDoubleClicked, 
        [this](DataObject * dataObject, int row) {
        qDebug() << sender();
        renderViewsLookAt(dataObject, static_cast<vtkIdType>(row));
    });

    updateSyncToggleMenu();
}

void SelectionHandler::addRenderView(RenderView * renderView)
{
    QAction * syncToggleAction = new QAction(renderView->windowTitle(), this);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);
    m_renderViews.insert(renderView, syncToggleAction);
    m_actionForInteractor.insert(renderView->interactorStyle(), syncToggleAction);
    connect(renderView, &QWidget::windowTitleChanged, syncToggleAction, &QAction::setText);
    connect(renderView->interactorStyle(), &IPickingInteractorStyle::cellPicked, this, &SelectionHandler::syncRenderAndTableViews);

    updateSyncToggleMenu();
}

void SelectionHandler::removeTableView(TableView * tableView)
{
    QAction * action = m_tableViews[tableView];

    m_tableViews.remove(tableView);

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::removeRenderView(RenderView * renderView)
{
    QAction * action = m_renderViews[renderView];

    m_renderViews.remove(renderView);
    m_actionForInteractor.remove(renderView->interactorStyle());

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::setSyncToggleMenu(QMenu * syncToggleMenu)
{
    m_syncToggleMenu = syncToggleMenu;
}

void SelectionHandler::syncRenderViewsWithTable(DataObject * dataObject, vtkIdType cellId)
{
    QAction * action = m_tableViews.value(static_cast<TableView*>(sender()), nullptr);
    assert(action);
    if (!action->isChecked())
        return;

    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObjects().contains(dataObject))
            it.key()->interactorStyle()->highlightCell(dataObject, cellId);
    }
}

void SelectionHandler::syncRenderAndTableViews(DataObject * dataObject, vtkIdType cellId)
{
    QAction * action = m_actionForInteractor.value(static_cast<IPickingInteractorStyle*>(sender()), nullptr);
    assert(action);
    if (!action->isChecked())
        return;

    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObjects().contains(dataObject))
            it.key()->interactorStyle()->highlightCell(dataObject, cellId);
    }
    for (auto it = m_tableViews.begin(); it != m_tableViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObject() == dataObject)
            it.key()->selectCell(cellId);
    }
}

void SelectionHandler::renderViewsLookAt(DataObject * dataObject, vtkIdType cellId)
{
    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked())
            it.key()->interactorStyle()->lookAtCell(dataObject, cellId);
    }
}

void SelectionHandler::updateSyncToggleMenu()
{
    m_syncToggleMenu->clear();

    m_syncToggleMenu->setEnabled(!m_tableViews.empty() || !m_renderViews.empty());


    m_syncToggleMenu->addActions(m_tableViews.values());
    m_syncToggleMenu->addSeparator();
    m_syncToggleMenu->addActions(m_renderViews.values());
}
