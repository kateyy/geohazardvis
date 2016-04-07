#pragma once

#include <core/types.h>
#include <core/t_QVTKWidgetFwd.h>

#include <gui/data_view/AbstractDataView.h>


class vtkRenderWindow;

class AbstractVisualizedData;
enum class ContentType;
enum class IndexType;
class RendererImplementation;


class GUI_API AbstractRenderView : public AbstractDataView
{
    Q_OBJECT

public:
    AbstractRenderView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

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
    /** @return visualization of dataObject in the specified sub-view. Returns nullptr, if the view currently doesn't visualize this data object*/
    virtual AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const = 0;
    /** @return the sub view index that contains the specified visualization, or -1 if this is not part of this view */
    virtual int subViewContaining(const AbstractVisualizedData & visualizedData) const = 0;

    /** The data object whose visualization is selected by the user. This visualization can be configured in the UI */
    virtual DataObject * selectedData() const = 0;
    /** The data visualization that is currently selected by the user. It can be configured in the UI. */
    virtual AbstractVisualizedData * selectedDataVisualization() const = 0;
    virtual void lookAtData(DataObject & dataObject, vtkIdType index, IndexType indexType, int subViewIndex = -1) = 0;
    virtual void lookAtData(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, int subViewIndex = -1) = 0;

    virtual unsigned int numberOfSubViews() const;
    unsigned int activeSubViewIndex() const;
    void setActiveSubView(unsigned int subViewIndex);

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;

    virtual RendererImplementation & implementation() const = 0;

    /** show/hide axes in case the render view currently contains data */
    void setEnableAxes(bool enabled);
    bool axesEnabled() const;

    void render();

    void showInfoText(const QString & text);
    QString infoText() const;

signals:
    /** emitted after changing the list of visible objects */
    void visualizationsChanged();

    /** Emitted after switching to a now implementation. 
    * Do not access pointers to the previous implementation when receiving this signal! */
    void implementationChanged();

    void selectedDataChanged(AbstractRenderView * renderView, DataObject * dataObject);
    void activeSubViewChanged(unsigned int activeSubViewIndex);

    void beforeDeleteVisualizations(const QList<AbstractVisualizedData *> & visualizations);

protected:
    QWidget * contentWidget() override;
    t_QVTKWidget & qvtkWidget();

    void showEvent(QShowEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;

    void selectionChangedEvent(DataObject * dataObject, vtkIdTypeArray * selection, IndexType indexType) override;

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
    t_QVTKWidget * m_qvtkWidget;
    bool m_onShowInitialized;

    bool m_axesEnabled;

    unsigned int m_activeSubViewIndex;
};
