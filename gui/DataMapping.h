#pragma once

#include <QObject>
#include <QList>
#include <QMap>


class MainWindow;
class DataObject;
class AbstractDataView;
class TableView;
class RenderView;
class RenderViewStrategySwitch;


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
    void addToRenderView(QList<DataObject *> dataObjects, RenderView * renderView);

    /** creates a render view but does not add it to the main window
      * This view can be part of composed views */
    RenderView * createDanglingRenderView();

    RenderView * focusedRenderView();

public slots:
    void setFocusedView(AbstractDataView * renderView);

signals:
    void renderViewsChanged(const QList<RenderView *> & widgets);
    void focusedRenderViewChanged(RenderView * renderView);

private slots:
    void focusNextRenderView();

    void tableClosed();
    void renderViewClosed();

private:
    bool askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects);

private:
    MainWindow & m_mainWindow;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    QMap<int, TableView *> m_tableViews;
    QMap<int, RenderView *> m_renderViews;

    RenderView * m_focusedRenderView;
};
