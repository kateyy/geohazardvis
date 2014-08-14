#pragma once

#include <QObject>
#include <QMap>

#include <vtkType.h>


class QItemSelection;
class QMenu;
class QAction;

class TableWidget;
class RenderWidget;
class DataObject;
class IPickingInteractorStyle;


class SelectionHandler : public QObject
{
    Q_OBJECT

public:
    static SelectionHandler & instance();

    /** consider this table view when updating selections */
    void addTableView(TableWidget * tableWidget);
    void removeTableView(TableWidget * tableWidget);
    /** consider this render view when updating selections */
    void addRenderView(RenderWidget * renderWidget);
    void removeRenderView(RenderWidget * renderWidget);

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
    QMap<TableWidget*, QAction*> m_tableWidgets;
    QMap<RenderWidget*, QAction*> m_renderWidgets;

    QMap<IPickingInteractorStyle *, QAction *> m_actionForInteractor;

    QMenu * m_syncToggleMenu;
};
