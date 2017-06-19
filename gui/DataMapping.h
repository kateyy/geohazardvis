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


/**
 * Interface that allows to create data views, docked into the application main window.
 *
 * The application will always have only one main window with render and table views attached to it
 * or floating around. That's why creation and handling of data view is handled at a central place.
 *
 * The usual setup is as follows:
 * Data sets loaded into the application are listed in the DataSetHandler and visible to the user
 * through the DataBrowser. The DataBrowser redirects user requests to DataMapping to open table or
 * render views. The application main window connects to the signals of the DataMapping
 * (renderViewCreated, etc.) so that it can embed newly opened views as dock widgets.
 * The DataMapping further creates a SelectionHandler which provides synchronized selections across
 * data views.
 *
 * This class also handles ownership if data views. In most cases this is implemented by passing
 * ownership to the Qt parent/child object system. However, especially in test setups, data views
 * are explicitly deleted by this class.
 */
class GUI_API DataMapping : public QObject
{
    Q_OBJECT

public:
    explicit DataMapping(DataSetHandler & dataSetHandler);
    ~DataMapping() override;
    void cleanup();

    DataSetHandler & dataSetHandler() const;
    SelectionHandler & selectionHandler();

    /**
     * Remove a list of data objects from data views, so that the objects can safely be deleted.
     */
    void removeDataObjects(const QList<DataObject *> & dataObjects);

    enum OpenFlag
    {
        NoFlags = 0x0,
        PlaceRight = 0x1,  // Place a new view to the right of existing views (default).
        PlaceBelow = 0x2   // Place a new view below existing views.
    };
    Q_DECLARE_FLAGS(OpenFlags, OpenFlag)

    void openInTable(DataObject * dataObject);
    /**
     * Opens a new render view and calls addToRenderView with the specified dataObjects on it.
     * @return Might return a nullptr, if the user requested to close the view during this function
     * call.
     */
    AbstractRenderView * openInRenderView(const QList<DataObject *> & dataObjects,
        OpenFlags openFlags = OpenFlag::NoFlags);
    /**
     * Open a data set in the specified render view.
     * @return Might return false, if the user requested to close the view during this function
     * call. This can happen for render view implementations calling QApplication::processEvents
     * during their setup, to increase interactivity.
     */
    bool addToRenderView(const QList<DataObject *> & dataObjects,
        AbstractRenderView * renderView,
        unsigned int subViewIndex = 0,
        OpenFlags openFlagsForFallbackView = OpenFlag::NoFlags);

    template<typename T>
    T * createRenderView(OpenFlags openFlags = OpenFlag::NoFlags);

    AbstractRenderView * createDefaultRenderViewType(OpenFlags openFlags = OpenFlag::NoFlags);

    AbstractRenderView * focusedRenderView();
    void setFocusedRenderView(AbstractDataView * renderView);

    QList<AbstractRenderView *> renderViews() const;
    QList<TableView *> tableViews() const;

signals:
    void renderViewCreated(AbstractRenderView * renderView, OpenFlags openFlags);
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

    void addRenderView(std::unique_ptr<AbstractRenderView> renderView,
        OpenFlags openFlags = OpenFlag::NoFlags);
    bool askForNewRenderView(const QString & rendererName, const QList<DataObject *> & relevantObjects);

private:
    DataSetHandler & m_dataSetHandler;

    std::unique_ptr<SelectionHandler> m_selectionHandler;

    int m_nextTableIndex;
    int m_nextRenderViewIndex;
    std::vector<std::unique_ptr<TableView>> m_tableViews;
    std::vector<std::unique_ptr<AbstractRenderView>> m_renderViews;

    AbstractRenderView * m_focusedRenderView;

private:
    Q_DISABLE_COPY(DataMapping)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DataMapping::OpenFlags)


template<typename T>
T * DataMapping::createRenderView(const OpenFlags openFlags)
{
    auto view = std::make_unique<T>(*this, m_nextRenderViewIndex++);
    auto ptr = view.get();

    addRenderView(std::move(view), openFlags);

    return ptr;
}
