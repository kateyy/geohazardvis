#pragma once

#include <QObject>
#include <QList>
#include <QMap>


class AbstractDataView;
class AbstractRenderView;
class DataObject;
class MainWindow;
class TableView;


class DataMapping : public QObject
{
    Q_OBJECT

public:
    DataMapping(MainWindow & mainWindow);
    ~DataMapping() override;
    static DataMapping & instance();

    void removeDataObjects(QList<DataObject *> dataObjects);

    void openInTable(DataObject * dataObject);
    /** Opens a new render view and calls addToRenderView with the specified dataObjects on it.
        @return Might return a nullptr, if the user requested to close the view during this function call. */
    AbstractRenderView * openInRenderView(QList<DataObject *> dataObjects);
    /** Open a data set in the specified render view.
        @return Might return false, if the user requested to close the view during this function call. 
            Some render view implementations call QApplication::processEvents to increase interactivity. */
    bool addToRenderView(const QList<DataObject *> & dataObjects, AbstractRenderView * renderView, unsigned int subViewIndex = 0);

    template<typename T>
    T * createRenderView();

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
    void addRenderView(AbstractRenderView * renderView);
    bool askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects);

private:
    MainWindow & m_mainWindow;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    QMap<int, TableView *> m_tableViews;
    QMap<int, AbstractRenderView *> m_renderViews;

    AbstractRenderView * m_focusedRenderView;
};


template<typename T>
T * DataMapping::createRenderView()
{
    auto view = new T(m_nextRenderViewIndex++);

    addRenderView(view);

    return view;
}
