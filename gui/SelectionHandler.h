#pragma once

#include <QObject>
#include <QSet>

#include <vtkType.h>


class QItemSelection;

class TableWidget;
class RenderWidget;
class DataObject;


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

private slots:
    void syncRenderViewsWithTable(DataObject * dataObject, vtkIdType cellId);
    void syncRenderAndTableViews(DataObject * dataObject, vtkIdType cellId);
    void renderViewsLookAt(DataObject * dataObject, vtkIdType cellId);

private:
    SelectionHandler();
    ~SelectionHandler() override;

private:
    QSet<TableWidget*> m_tableWidgets;
    QSet<RenderWidget*> m_renderWidgets;
};
