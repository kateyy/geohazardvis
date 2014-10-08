#pragma once

#include <QObject>
#include <QList>
#include <QMap>


class MainWindow;
class DataObject;
class AbstractDataView;
class TableView;
class RenderView;
class RenderViewSwitch;


class DataMapping : public QObject
{
    Q_OBJECT

public:
    DataMapping(MainWindow & mainWindow);
    ~DataMapping() override;
    static DataMapping & instance();

    void removeDataObjects(QList<DataObject *> dataObjects);

    void openInTable(DataObject * dataObject);
    RenderView * openInRenderView(QList<DataObject *> dataObjects);
    void addToRenderView(QList<DataObject *> dataObjects, int renderView);

    RenderView * focusedRenderView();

signals:
    void renderViewsChanged(const QList<RenderView *> & widgets);
    void focusedRenderViewChanged(RenderView * renderView);

private slots:
    void setFocusedView(AbstractDataView * renderView);
    void focusNextRenderView();

    void tableClosed();
    void renderViewClosed();

private:
    MainWindow & m_mainWindow;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    QMap<int, TableView *> m_tableViews;
    QMap<int, RenderView *> m_renderViews;
    QMap<RenderView *, RenderViewSwitch *> m_renderViewSwitches;

    RenderView * m_focusedRenderView;
};
