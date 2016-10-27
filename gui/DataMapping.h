#pragma once

#include <memory>
#include <vector>

#include <QObject>

#include <gui/gui_api.h>


class QDockWidget;
template<typename T> class QList;

class AbstractDataView;
class AbstractRenderView;
class DataObject;
class DataSetHandler;
class SelectionHandler;
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
    void setFocusedRenderView(AbstractDataView * renderView);

    QList<AbstractRenderView *> renderViews() const;
    QList<TableView *> tableViews() const;

signals:
    void renderViewCreated(AbstractRenderView * renderView);
    void focusedRenderViewChanged(AbstractRenderView * renderView);
    void tableViewCreated(TableView * tableView, QDockWidget * dockTabifyPartner);

private:
    void focusNextRenderView();

    void tableClosed();
    void renderViewClosed();
    /** Pass responsibility for deletion to Qt, calling QObject::deleteLater() */
    void renderViewDeleteLater(AbstractRenderView * view);
    void tableViewDeleteLater(TableView * view);
    template<typename View_t, typename Vector_t>
    void deleteLaterFrom(View_t * view, Vector_t & vector);

    void addRenderView(std::unique_ptr<AbstractRenderView> renderView);
    bool askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects);

private:
    DataSetHandler & m_dataSetHandler;
    bool m_deleting;

    std::unique_ptr<SelectionHandler> m_selectionHandler;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    std::vector<std::unique_ptr<TableView>> m_tableViews;
    std::vector<std::unique_ptr<AbstractRenderView>> m_renderViews;

    AbstractRenderView * m_focusedRenderView;

private:
    Q_DISABLE_COPY(DataMapping)
};


template<typename T>
T * DataMapping::createRenderView()
{
    auto view = std::make_unique<T>(*this, m_nextRenderViewIndex++);
    auto ptr = view.get();

    addRenderView(std::move(view));

    return ptr;
}
