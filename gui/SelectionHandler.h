#pragma once

#include <QObject>
#include <QMap>

#include <vtkType.h>


class QItemSelection;
class QMenu;
class QAction;

class AbstractDataView;
class TableView;
class RenderView;
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
    void addRenderView(RenderView * renderView);
    void removeRenderView(RenderView * renderView);

    void setSyncToggleMenu(QMenu * syncToggleMenu);

private slots:
    void syncRenderViewsWithTable(DataObject * dataObject, vtkIdType cellId);
    void syncRenderAndTableViews(DataObject * dataObject, vtkIdType cellId);
    void renderViewsLookAt(DataObject * dataObject, vtkIdType cellId);

private:
    SelectionHandler();
    ~SelectionHandler() override;

    void updateSyncToggleMenu();

private:
    QMap<TableView*, QAction*> m_tableViews;
    QMap<RenderView*, QAction*> m_renderViews;

    QMap<IPickingInteractorStyle *, QAction *> m_actionForInteractor;

    QMenu * m_syncToggleMenu;
};
