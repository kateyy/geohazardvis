#include "SelectionHandler.h"

#include <cassert>

#include <QTableView>
#include <QAction>
#include <QMenu>

#include <core/data_objects/DataObject.h>

#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/TableView.h>


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
    QAction * syncToggleAction = addAbstractDataView(tableView);

    m_tableViews.insert(tableView, syncToggleAction);
    updateSyncToggleMenu();
    
    connect(tableView, &TableView::itemDoubleClicked, 
        [this](DataObject * dataObject, vtkIdType itemId) {
        renderViewsLookAt(dataObject, itemId);
    });
}

void SelectionHandler::addRenderView(AbstractRenderView * renderView)
{
    QAction * syncToggleAction = addAbstractDataView(renderView);

    m_renderViews.insert(renderView, syncToggleAction);
    updateSyncToggleMenu();
}

QAction * SelectionHandler::addAbstractDataView(AbstractDataView * dataView)
{
    QAction * syncToggleAction = new QAction(dataView->windowTitle(), this);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);

    connect(dataView, &AbstractDataView::windowTitleChanged, syncToggleAction, &QAction::setText);
    connect(dataView, &AbstractDataView::objectPicked, this, &SelectionHandler::hightlightSelection);

    return syncToggleAction;
}

void SelectionHandler::removeTableView(TableView * tableView)
{
    QAction * action = m_tableViews[tableView];

    m_tableViews.remove(tableView);

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::removeRenderView(AbstractRenderView * renderView)
{
    QAction * action = m_renderViews[renderView];

    m_renderViews.remove(renderView);

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::setSyncToggleMenu(QMenu * syncToggleMenu)
{
    m_syncToggleMenu = syncToggleMenu;
}

void SelectionHandler::hightlightSelection(DataObject * dataObject, vtkIdType highlightedItemId)
{
    QAction * action = nullptr;
    if (TableView * table = dynamic_cast<TableView *>(sender()))
        action = m_tableViews[table];
    else
    {
        assert(dynamic_cast<AbstractRenderView *>(sender()));
        action = m_renderViews.value(static_cast<AbstractRenderView *>(sender()));
    }
    assert(action);

    if (!action->isChecked())
        return;


    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObjects().contains(dataObject))
            it.key()->setHighlightedId(dataObject, highlightedItemId);
    }
    for (auto it = m_tableViews.begin(); it != m_tableViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObject() == dataObject)
            it.key()->setHighlightedId(dataObject, highlightedItemId);
    }
}

void SelectionHandler::renderViewsLookAt(DataObject * dataObject, vtkIdType itemId)
{
    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked())
            if (it.key()->dataObjects().contains(dataObject))
                it.key()->lookAtData(dataObject, itemId);
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
