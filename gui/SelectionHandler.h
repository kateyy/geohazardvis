#pragma once

#include <QObject>
#include <QMap>

#include <core/types.h>


class QAction;
class QItemSelection;
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

    QAction * addAbstractDataView(AbstractDataView * dataView);

private:
    QMap<TableView *, QAction *> m_tableViews;
    QMap<AbstractRenderView *, QAction *> m_renderViews;

    QMenu * m_syncToggleMenu;

private:
    Q_DISABLE_COPY(SelectionHandler)
};
