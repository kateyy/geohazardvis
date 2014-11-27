#pragma once

#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/AbstractDataView.h>


class vtkRenderWindow;

enum class ContentType;
class DataObject;
class AbstractVisualizedData;

class Ui_RenderView;
class RendererImplementation;


class GUI_API RenderView : public AbstractDataView
{
    Q_OBJECT

public:
    RenderView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

    ContentType contentType() const;

    /** Add data objects to the view or make already added objects visible again.
        @param incompatibleObjects List of objects that are not compatible with current content (e.g. 2D vs. 3D data). */
    void addDataObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects);
    /** remove rendered representations of data objects, don't delete data and settings */
    void hideDataObjects(const QList<DataObject *> & dataObjects);
    /** check if the this objects is currently rendered */
    bool contains(DataObject * dataObject) const;
    /** remove rendered representations and all references to the data objects */
    void removeDataObjects(const QList<DataObject *> & dataObjects);
    QList<DataObject *> dataObjects() const;
    const QList<AbstractVisualizedData *> & contents() const;

    DataObject * highlightedData() const;
    AbstractVisualizedData * highlightedContent() const;
    void lookAtData(DataObject * dataObject, vtkIdType itemId);

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;

    /** show/hide axes in case the render view currently contains data */
    void setEnableAxes(bool enabled);
    bool axesEnabled() const;

    bool contains3dData() const;

    // remove from public interface as soon as possible
    RendererImplementation & implementation() const;

signals:
    /** emitted after changing the list of visible objects */
    void contentChanged();
    /** emitted when loading data into an empty view
        @param dataObjects List of objects that are requested for visualization. */
    void resetImplementation(const QList<DataObject *> & dataObjects);

    void selectedDataChanged(RenderView * renderView, DataObject * dataObject);

    void beforeDeleteContent(AbstractVisualizedData * content);

public slots:
    void render();

    void ShowInfo(const QStringList &info);

protected:
    QWidget * contentWidget() override;
    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

private:
    // GUI / rendering tools
    void setupRenderer();

    // data handling

    AbstractVisualizedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    // remove some data objects from internal lists
    // @return list of dangling rendered data object that you have to delete.
    QList<AbstractVisualizedData *> removeFromInternalLists(QList<DataObject *> dataObjects = {});


private slots:
    /** update configuration widgets to focus on my content. */
    void updateGuiForContent();
    void updateGuiForSelectedData(AbstractVisualizedData * content);
    void updateGuiForRemovedData();

private:
    Ui_RenderView * m_ui;
    RendererImplementation * m_implementation;

    // rendered representations of data objects for this view
    QList<AbstractVisualizedData *> m_contents;
    // objects that were loaded to the GPU but are currently not rendered 
    QList<AbstractVisualizedData *> m_contentCache;
    QMap<DataObject *, AbstractVisualizedData *> m_dataObjectToVisualization;

    // visualization and annotation
    bool m_axesEnabled;
};
