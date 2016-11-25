#include "SelectionHandler.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include <QAction>
#include <QList>
#include <QMenu>

#include <core/ThirdParty/alphanum.hpp>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/TableView.h>


SelectionHandler::SelectionHandler()
    : m_syncToggleMenu{ nullptr }
{
}

SelectionHandler::~SelectionHandler() = default;

void SelectionHandler::addTableView(TableView * tableView)
{
    m_tableViewActions.emplace(tableView, createActionForDataView(tableView));
    updateSyncToggleMenu();
    
    connect(tableView, &TableView::itemDoubleClicked,
        this, &SelectionHandler::renderViewsLookAt);
}

void SelectionHandler::addRenderView(AbstractRenderView * renderView)
{
    m_renderViewActions.emplace(renderView, createActionForDataView(renderView));
    updateSyncToggleMenu();
}

std::unique_ptr<QAction> SelectionHandler::createActionForDataView(AbstractDataView * dataView) const
{
    auto syncToggleAction = std::make_unique<QAction>(dataView->windowTitle(), nullptr);
    syncToggleAction->setCheckable(true);
    syncToggleAction->setChecked(true);

    connect(dataView, &AbstractDataView::windowTitleChanged, syncToggleAction.get(), &QAction::setText);
    connect(dataView, &AbstractDataView::selectionChanged, this, &SelectionHandler::updateSelection);

    return syncToggleAction;
}

void SelectionHandler::removeTableView(TableView * tableView)
{
    auto it = m_tableViewActions.find(tableView);
    if (it == m_tableViewActions.end())
    {
        return;
    }

    auto action = std::move(it->second);

    m_tableViewActions.erase(it);

    updateSyncToggleMenu();
}

void SelectionHandler::removeRenderView(AbstractRenderView * renderView)
{
    auto it = m_renderViewActions.find(renderView);
    if (it == m_renderViewActions.end())
    {
        return;
    }

    auto action = std::move(it->second);

    m_renderViewActions.erase(it);

    updateSyncToggleMenu();
}

void SelectionHandler::setSyncToggleMenu(QMenu * syncToggleMenu)
{
    if (m_syncToggleMenu)
    {
        m_syncToggleMenu->clear();
        m_renderViewActions.clear();
        m_tableViewActions.clear();
    }

    m_syncToggleMenu = syncToggleMenu;

    updateSyncToggleMenu();
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
        action = m_tableViewActions[table].get();
    }
    else
    {
        assert(dynamic_cast<AbstractRenderView *>(sender()));
        action = m_renderViewActions[static_cast<AbstractRenderView *>(sender())].get();
    }
    assert(action);

    if (!action->isChecked())
    {
        return;
    }


    for (auto && viewAction : m_renderViewActions)
    {
        auto && view = viewAction.first;
        auto && vAction = viewAction.second;

        if (vAction->isChecked() && view->dataObjects().contains(selection.dataObject))
        {
            view->setSelection(selection);
        }
    }
    for (auto && viewAction : m_tableViewActions)
    {
        auto && view = viewAction.first;
        auto && vAction = viewAction.second;

        if (vAction->isChecked() && view->dataObject() == selection.dataObject)
        {
            view->setSelection(selection);
        }
    }
}

void SelectionHandler::renderViewsLookAt(const DataSelection & selection)
{
    if (!selection.dataObject)
    {
        return;
    }

    for (auto && viewAction : m_renderViewActions)
    {
        auto && view = viewAction.first;
        auto && action = viewAction.second;

        if (action->isChecked()
            && view->dataObjects().contains(selection.dataObject))
        {
            view->lookAtData(selection);
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

    m_syncToggleMenu->setEnabled(!m_tableViewActions.empty() || !m_renderViewActions.empty());

    QList<QAction *> sortedActions;
    auto nameSorter = [] (const QAction * lhs, const QAction * rhs) -> bool
    {
        return doj::alphanum_less<QString>()(lhs->text(), rhs->text());
    };

    m_syncToggleMenu->addAction("Table Views")->setEnabled(false);
    for (auto && it : m_tableViewActions)
    {
        sortedActions.push_back(it.second.get());
    }
    std::sort(sortedActions.begin(), sortedActions.end(), nameSorter);
    m_syncToggleMenu->addActions(sortedActions);
    sortedActions.clear();

    m_syncToggleMenu->addSeparator();
    m_syncToggleMenu->addAction("Render Views")->setEnabled(false);
    for (auto && it : m_renderViewActions)
    {
        sortedActions.push_back(it.second.get());
    }
    std::sort(sortedActions.begin(), sortedActions.end(), nameSorter);
    m_syncToggleMenu->addActions(sortedActions);
}
