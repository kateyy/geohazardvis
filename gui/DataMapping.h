#pragma once

#include <memory>

#include <QObject>
#include <QMap>

#include <gui/gui_api.h>


class QDockWidget;
template<typename T> class QList;

class AbstractDataView;
class AbstractRenderView;
class DataObject;
class DataSetHandler;
class SelectionHandler;
class RenderView;
class TableView;


class GUI_API DataMapping : public QObject
{
    Q_OBJECT

public:
    explicit DataMapping(DataSetHandler & dataSetHandler);
    ~DataMapping() override;

    DataSetHandler & dataSetHandler() const;
    SelectionHandler & selectionHandler();

    void removeDataObjects(const QList<DataObject *> & dataObjects);

    void openInTable(DataObject * dataObject);
    /** Opens a new render view and calls addToRenderView with the specified dataObjects on it.
        @return Might return a nullptr, if the user requested to close the view during this function call. */
    AbstractRenderView * openInRenderView(const QList<DataObject *> & dataObjects);
    /** Open a data set in the specified render view.
        @return Might return false, if the user requested to close the view during this function call. 
            Some render view implementations call QApplication::processEvents to increase interactivity. */
    bool addToRenderView(const QList<DataObject *> & dataObjects, AbstractRenderView * renderView, unsigned int subViewIndex = 0);

    template<typename T>
    T * createRenderView();

    AbstractRenderView * createDefaultRenderViewType();

    AbstractRenderView * focusedRenderView();

    QList<AbstractRenderView *> renderViews() const;
    QList<TableView *> tableViews() const;

public:
    void setFocusedView(AbstractDataView * renderView);

signals:
    void renderViewCreated(AbstractRenderView * renderView);
    void tableViewCreated(TableView * tableView, QDockWidget * dockTabifyPartner);
    void renderViewsChanged(const QList<AbstractRenderView *> & widgets);
    void focusedRenderViewChanged(AbstractRenderView * renderView);

private:
    void focusNextRenderView();

    void tableClosed();
    void renderViewClosed();

private:
    void addRenderView(AbstractRenderView * renderView);
    bool askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects);

private:
    DataSetHandler & m_dataSetHandler;

    std::unique_ptr<SelectionHandler> m_selectionHandler;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    QMap<int, TableView *> m_tableViews;
    QMap<int, AbstractRenderView *> m_renderViews;

    AbstractRenderView * m_focusedRenderView;

private:
    Q_DISABLE_COPY(DataMapping)
};


template<typename T>
T * DataMapping::createRenderView()
{
    auto view = new T(*this, m_nextRenderViewIndex++);

    addRenderView(view);

    return view;
}
