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

#pragma once

#include <map>
#include <memory>

#include <QObject>

#include <core/types.h>


class QAction;
class QMenu;

class AbstractDataView;
class AbstractRenderView;
class TableView;


class SelectionHandler : public QObject
{
public:
    SelectionHandler();
    ~SelectionHandler() override;

    /** consider this table view when updating selections */
    void addTableView(TableView * tableView);
    void removeTableView(TableView * tableView);
    /** consider this render view when updating selections */
    void addRenderView(AbstractRenderView * renderView);
    void removeRenderView(AbstractRenderView * renderView);

    void setSyncToggleMenu(QMenu * syncToggleMenu);

private:
    void updateSelection(AbstractDataView * sourceView, const DataSelection & selection);
    void renderViewsLookAt(const DataSelection & selection);

    void updateSyncToggleMenu();

    std::unique_ptr<QAction> createActionForDataView(AbstractDataView * dataView) const;

private:
    std::map<TableView *, std::unique_ptr<QAction>> m_tableViewActions;
    std::map<AbstractRenderView *, std::unique_ptr<QAction>> m_renderViewActions;

    QMenu * m_syncToggleMenu;

private:
    Q_DISABLE_COPY(SelectionHandler)
};
