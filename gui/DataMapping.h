#pragma once

#include <memory>

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

    void addDataObject(std::shared_ptr<DataObject> input);

    void openInTable(std::shared_ptr<DataObject> representation);
    void openInRenderView(std::shared_ptr<DataObject> representation);
    void addToRenderView(std::shared_ptr<DataObject> representation, int renderView);

signals:
    void renderViewsChanged(QList<RenderWidget*> widgets);

private slots:
    void tableClosed();
    void renderWidgetClosed();

private:
    MainWindow & m_mainWindow;

    QList<std::shared_ptr<DataObject>> m_dataObject;
    int m_nextTableIndex;
    int m_nextRenderWidgetIndex;
    QMap<int, TableWidget*> m_tableWidgets;
    QMap<int, RenderWidget*> m_renderWidgets;
};
