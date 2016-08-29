#include "SelectionHandler.h"

#include <cassert>

#include <QTableView>
#include <QAction>
#include <QMenu>

#include <core/data_objects/DataObject.h>

#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/TableView.h>


SelectionHandler::SelectionHandler()
    : m_syncToggleMenu{ nullptr }
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
    auto syncToggleAction = addAbstractDataView(tableView);

    m_tableViews.insert(tableView, syncToggleAction);
    updateSyncToggleMenu();
    
    connect(tableView, &TableView::itemDoubleClicked,
        this, &SelectionHandler::renderViewsLookAt);
}

void SelectionHandler::addRenderView(AbstractRenderView * renderView)
{
    auto syncToggleAction = addAbstractDataView(renderView);

    m_renderViews.insert(renderView, syncToggleAction);
    updateSyncToggleMenu();
}

QAction * SelectionHandler::addAbstractDataView(AbstractDataView * dataView)
{
    auto syncToggleAction = new QAction(dataView->windowTitle(), this);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);

    connect(dataView, &AbstractDataView::windowTitleChanged, syncToggleAction, &QAction::setText);
    connect(dataView, &AbstractDataView::selectionChanged, this, &SelectionHandler::updateSelection);

    return syncToggleAction;
}

void SelectionHandler::removeTableView(TableView * tableView)
{
    auto action = m_tableViews[tableView];

    m_tableViews.remove(tableView);

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::removeRenderView(AbstractRenderView * renderView)
{
    auto action = m_renderViews[renderView];

    m_renderViews.remove(renderView);

    updateSyncToggleMenu();

    delete action;
}

void SelectionHandler::setSyncToggleMenu(QMenu * syncToggleMenu)
{
    m_syncToggleMenu = syncToggleMenu;
}

void SelectionHandler::updateSelection(AbstractDataView * /*sourceView*/, const DataSelection & selection)
{
    if (selection.isEmpty())
    {
        return;
    }

    QAction * action = nullptr;
    if (auto table = dynamic_cast<TableView *>(sender()))
    {
        action = m_tableViews[table];
    }
    else
    {
        assert(dynamic_cast<AbstractRenderView *>(sender()));
        action = m_renderViews.value(static_cast<AbstractRenderView *>(sender()));
    }
    assert(action);

    if (!action->isChecked())
    {
        return;
    }


    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObjects().contains(selection.dataObject))
        {
            it.key()->setSelection(selection);
        }
    }
    for (auto it = m_tableViews.begin(); it != m_tableViews.end(); ++it)
    {
        if (it.value()->isChecked() && it.key()->dataObject() == selection.dataObject)
        {
            it.key()->setSelection(selection);
        }
    }
}

void SelectionHandler::renderViewsLookAt(const DataSelection & selection)
{
    if (!selection.dataObject)
    {
        return;
    }

    for (auto it = m_renderViews.begin(); it != m_renderViews.end(); ++it)
    {
        if (it.value()->isChecked())
        {
            if (it.key()->dataObjects().contains(selection.dataObject))
            {
                it.key()->lookAtData(selection);
            }
        }
    }
}

void SelectionHandler::updateSyncToggleMenu()
{
    if (!m_syncToggleMenu)  // for standalone / testing usage
    {
        return;
    }

    m_syncToggleMenu->clear();

    m_syncToggleMenu->setEnabled(!m_tableViews.empty() || !m_renderViews.empty());


    m_syncToggleMenu->addActions(m_tableViews.values());
    m_syncToggleMenu->addSeparator();
    m_syncToggleMenu->addActions(m_renderViews.values());
}
