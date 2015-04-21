#pragma once

#include <QObject>
#include <QList>
#include <QMap>


class MainWindow;
class DataObject;
class AbstractDataView;
class AbstractRenderView;
class TableView;
class RenderView;


class DataMapping : public QObject
{
    Q_OBJECT

public:
    DataMapping(MainWindow & mainWindow);
    ~DataMapping() override;
    static DataMapping & instance();

    void removeDataObjects(QList<DataObject *> dataObjects);

    void openInTable(DataObject * dataObject);
    AbstractRenderView * openInRenderView(QList<DataObject *> dataObjects);
    void addToRenderView(QList<DataObject *> dataObjects, AbstractRenderView * renderView, unsigned int subViewIndex = 0);

    AbstractRenderView * focusedRenderView();

public slots:
    void setFocusedView(AbstractDataView * renderView);

signals:
    void renderViewsChanged(const QList<AbstractRenderView *> & widgets);
    void focusedRenderViewChanged(AbstractRenderView * renderView);

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
    QMap<int, AbstractRenderView *> m_renderViews;

    AbstractRenderView * m_focusedRenderView;
};
