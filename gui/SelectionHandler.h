#pragma once

#include <QObject>
#include <QMap>

#include <vtkType.h>


class QItemSelection;
class QMenu;
class QAction;

class AbstractDataView;
class AbstractRenderView;
class TableView;
class DataObject;
class IPickingInteractorStyle;


class SelectionHandler : public QObject
{
    Q_OBJECT

public:
    static SelectionHandler & instance();

    /** consider this table view when updating selections */
    void addTableView(TableView * tableView);
    void removeTableView(TableView * tableView);
    /** consider this render view when updating selections */
    void addRenderView(AbstractRenderView * renderView);
    void removeRenderView(AbstractRenderView * renderView);

    void setSyncToggleMenu(QMenu * syncToggleMenu);

private slots:
    void hightlightSelection(DataObject * dataObject, vtkIdType highlightedItemId);

    void renderViewsLookAt(DataObject * dataObject, vtkIdType itemId);

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
