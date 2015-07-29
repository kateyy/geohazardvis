#pragma once

#include <gui/data_view/AbstractDataView.h>


class vtkRenderWindow;

class AbstractVisualizedData;
enum class ContentType;
class RendererImplementation;


class GUI_API AbstractRenderView : public AbstractDataView
{
    Q_OBJECT

public:
    AbstractRenderView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

    bool isTable() const override;
    bool isRenderer() const override;

    virtual ContentType contentType() const = 0;

    /** Add data objects to the view or make already added objects visible again.
        @param incompatibleObjects Objects that cannot be added to the current contents. 
        @param subViewIndex For composed render view, the sub view that the objects will be added to */
    void showDataObjects(
        const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex = 0);
    /** hide rendered representations of data objects, keep render data and settings */
    void hideDataObjects(const QList<DataObject *> & dataObjects, unsigned int subViewIndex = 0);
    /** check if the this objects is currently rendered
        @param subViewIndex The index of the sub view in composed render views. -1 means to check for the
        DataObject in any of the sub views */
    bool contains(DataObject * dataObject, int subViewIndex = -1) const;
    /** Remove rendered representations and all references to the data objects.
        For composed views, all sub views will be cleared from the specified data objects */
    void prepareDeleteData(const QList<DataObject *> & dataObjects);
    /** DataObjects visible in the specified subViewIndex, or in any of the sub views (-1) */
    QList<DataObject *> dataObjects(int subViewIndex = -1) const;
    QList<AbstractVisualizedData *> visualizations(int subViewIndex = -1) const;
    /** @return visualization of dataObject in the specified sub-view. Returns nullptr, if the view currently doesn't visualize the data object*/
    virtual AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const = 0;

    /** The data object whose visualization is selected by the user. This visualization can be configured in the UI */
    virtual DataObject * selectedData() const = 0;
    /** The data visualization that is currently selected by the user. It can be configured in the UI. */
    virtual AbstractVisualizedData * selectedDataVisualization() const = 0;
    virtual void lookAtData(DataObject * dataObject, vtkIdType itemId) = 0;

    virtual unsigned int numberOfSubViews() const;
    unsigned int activeSubViewIndex() const;
    void setActiveSubView(unsigned int subViewIndex);

    virtual vtkRenderWindow * renderWindow() = 0;
    virtual const vtkRenderWindow * renderWindow() const = 0;

    virtual RendererImplementation & implementation() const = 0;

    /** show/hide axes in case the render view currently contains data */
    void setEnableAxes(bool enabled);
    bool axesEnabled() const;

    void render();

    void ShowInfo(const QStringList &info);

signals:
    /** emitted after changing the list of visible objects */
    void visualizationsChanged();

    void selectedDataChanged(AbstractRenderView * renderView, DataObject * dataObject);
    void activeSubViewChanged(unsigned int activeSubViewIndex);

    void beforeDeleteVisualization(AbstractVisualizedData * content);

protected:
    bool eventFilter(QObject * watched, QEvent * event) override;
    void showEvent(QShowEvent * event) override;

    virtual void showDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) = 0;
    virtual void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex) = 0;
    virtual QList<DataObject *> dataObjectsImpl(int subViewIndex) const = 0;
    virtual void prepareDeleteDataImpl(const QList<DataObject *> & dataObjects) = 0;
    virtual QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const = 0;

    virtual void activeSubViewChangedEvent(unsigned int subViewIndex);
    virtual void axesEnabledChangedEvent(bool enabled) = 0;

private:
    bool m_isInitialized;

    bool m_axesEnabled;

    unsigned int m_activeSubViewIndex;
};
