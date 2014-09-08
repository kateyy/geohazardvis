#pragma once

#include <QObject>
#include <QList>
#include <QMap>


class MainWindow;
class DataObject;
class AbstractDataView;
class TableView;
class RenderView;


class DataMapping : public QObject
{
    Q_OBJECT

public:
    DataMapping(MainWindow & mainWindow);
    ~DataMapping() override;

    void addDataObjects(QList<DataObject *> dataObjects);
    void removeDataObjects(QList<DataObject *> dataObjects);

    void openInTable(DataObject * dataObject);
    RenderView * openInRenderView(QList<DataObject *> dataObjects);
    void addToRenderView(QList<DataObject *> dataObjects, int renderView);

    RenderView * focusedRenderView();

signals:
    void renderViewsChanged(QList<RenderView*> widgets);
    void focusedRenderViewChanged(RenderView * renderView);

private slots:
    void setFocusedView(AbstractDataView * renderView);

    void tableClosed();
    void renderViewClosed();

private:
    MainWindow & m_mainWindow;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    QMap<int, TableView*> m_tableViews;
    QMap<int, RenderView*> m_renderViews;

    RenderView * m_focusedRenderView;
};
