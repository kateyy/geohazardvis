#pragma once

#include <QObject>
#include <QList>
#include <QMap>


class MainWindow;
class DataObject;
class TableWidget;
class RenderWidget;


class DataMapping : public QObject
{
    Q_OBJECT

public:
    DataMapping(MainWindow & mainWindow);
    ~DataMapping() override;

    void addDataObjects(QList<DataObject *> dataObjects);
    void removeDataObjects(QList<DataObject *> dataObjects);

    void openInTable(DataObject * dataObject);
    void openInRenderView(QList<DataObject *> dataObjects);
    void addToRenderView(QList<DataObject *> dataObjects, int renderView);

    RenderWidget * focusedRenderView();

signals:
    void renderViewsChanged(QList<RenderWidget*> widgets);
    void focusedRenderViewChanged(RenderWidget * renderView);

private slots:
    void setFocusedRenderView(RenderWidget * renderView);

    void tableClosed();
    void renderWidgetClosed();

private:
    MainWindow & m_mainWindow;

    QList<DataObject *> m_dataObject;
    int m_nextTableIndex;
    int m_nextRenderWidgetIndex;
    QMap<int, TableWidget*> m_tableWidgets;
    QMap<int, RenderWidget*> m_renderWidgets;

    RenderWidget * m_focusedRenderView;
};
