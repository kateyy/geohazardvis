#pragma once

#include <vector>

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
    void addDataObjects(
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
    void removeDataObjects(const QList<DataObject *> & dataObjects);
    /** DataObjects visible in the specified subViewIndex, or in any of the sub views (-1) */
    QList<DataObject *> dataObjects(int subViewIndex = -1) const;
    QList<AbstractVisualizedData *> visualizations(int subViewIndex = -1) const;

    /** The data object whose visualization is selected by the user. This visualization can be configured in the UI */
    virtual DataObject * selectedData() const = 0;
    /** The data visualization that is currently selected by the user. It can be configured in the UI. */
    virtual AbstractVisualizedData * selectedDataVisualization() const = 0;
    virtual void lookAtData(DataObject * dataObject, vtkIdType itemId) = 0;

    /** The implementation of the render view the user is currently interacting with.
        For composed render views, multiple implementations might be used internally. 
        Simple/composed render views might switch between implementations at runtime, depending on their 
        current contents and visualization styles. */
    virtual RendererImplementation & selectedViewImplementation() = 0;
    virtual const RendererImplementation & selectedViewImplementation() const = 0;

    virtual unsigned int numberOfSubViews() const;

    virtual vtkRenderWindow * renderWindow() = 0;
    virtual const vtkRenderWindow * renderWindow() const = 0;

    /** show/hide axes in case the render view currently contains data */
    void setEnableAxes(bool enabled);
    bool axesEnabled() const;

signals:
    /** emitted after changing the list of visible objects */
    void visualizationsChanged();

    void selectedDataChanged(AbstractRenderView * renderView, DataObject * dataObject);

    void beforeDeleteVisualization(AbstractVisualizedData * content);

public slots:
    virtual void render() = 0;

    void ShowInfo(const QStringList &info);

protected:
    virtual void addDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) = 0;
    virtual void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex) = 0;
    virtual void removeDataObjectsImpl(const QList<DataObject *> & dataObjects) = 0;
    virtual QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const = 0;

    virtual void axesEnabledChangedEvent(bool enabled) = 0;

private:
    std::vector<QList<DataObject *>> m_dataObjects;

    bool m_axesEnabled;
};
