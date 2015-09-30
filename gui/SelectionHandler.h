#pragma once

#include <QObject>
#include <QMap>

#include <vtkType.h>


class QAction;
class QItemSelection;
class QMenu;

class AbstractDataView;
class AbstractRenderView;
class DataObject;
enum class IndexType;
class TableView;


class SelectionHandler : public QObject
{
public:
    static SelectionHandler & instance();

    /** consider this table view when updating selections */
    void addTableView(TableView * tableView);
    void removeTableView(TableView * tableView);
    /** consider this render view when updating selections */
    void addRenderView(AbstractRenderView * renderView);
    void removeRenderView(AbstractRenderView * renderView);

    void setSyncToggleMenu(QMenu * syncToggleMenu);

private:
    void updateSelection(DataObject * dataObject, vtkIdType index, IndexType indexType);
    void renderViewsLookAt(DataObject * dataObject, vtkIdType index, IndexType indexType);

private:
    SelectionHandler();
    ~SelectionHandler() override;

    void updateSyncToggleMenu();

    QAction * addAbstractDataView(AbstractDataView * dataView);

private:
    QMap<TableView *, QAction *> m_tableViews;
    QMap<AbstractRenderView *, QAction *> m_renderViews;

    QMenu * m_syncToggleMenu;
};
