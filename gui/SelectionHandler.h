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
