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

    void addDataObject(DataObject * input);

    void openInTable(DataObject * dataObject);
    void openInRenderView(DataObject * dataObject);
    void addToRenderView(DataObject * dataObject, int renderView);

signals:
    void renderViewsChanged(QList<RenderWidget*> widgets);

private slots:
    void tableClosed();
    void renderWidgetClosed();

private:
    MainWindow & m_mainWindow;

    QList<DataObject *> m_dataObject;
    int m_nextTableIndex;
    int m_nextRenderWidgetIndex;
    QMap<int, TableWidget*> m_tableWidgets;
    QMap<int, RenderWidget*> m_renderWidgets;
};
