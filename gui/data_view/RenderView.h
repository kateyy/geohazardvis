#pragma once

#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/AbstractDataView.h>


class vtkRenderer;
class vtkRenderWindow;
class vtkPolyData;
class vtkCubeAxesActor;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkActor;
class vtkLightKit;

class DataObject;
class RenderedData;

class ScalarToColorMapping;
class PickingInteractorStyleSwitch;
class IPickingInteractorStyle;
class Ui_RenderView;
class RenderViewStrategy;


class GUI_API RenderView : public AbstractDataView
{
    Q_OBJECT

public:
    RenderView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

    /** Add data objects to the view or make already added objects visible again.
        @param incompatibleObjects List of objects that are not compatible with current content (e.g. 2D vs. 3D data). */
    void addDataObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects);
    /** remove rendered representations of data objects, don't delete data and settings */
    void hideDataObjects(const QList<DataObject *> & dataObjects);
    /** check if the this objects is currently rendered */
    bool isVisible(DataObject * dataObject) const;
    /** remove rendered representations and all references to the data objects */
    void removeDataObjects(const QList<DataObject *> & dataObjects);
    QList<DataObject *> dataObjects() const;
    const QList<RenderedData *> & renderedData() const;

    DataObject * highlightedData() const;
    RenderedData * highlightedRenderedData() const;

    ScalarToColorMapping * scalarMapping();

    const double * getDataBounds() const;
    void getDataBounds(double bounds[6]) const;

    vtkRenderer * renderer();
    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;
    
    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;
    PickingInteractorStyleSwitch * interactorStyleSwitch();

    vtkLightKit * lightKit();
    vtkScalarBarWidget * colorLegendWidget();

    vtkCubeAxesActor * axesActor();
    /** show/hide axes in case the render view currently contains data */
    void setEnableAxes(bool enabled);
    bool axesEnabled() const;

    bool contains3dData() const;

signals:
    /** emitted after changing the list of visible objects */
    void renderedDataChanged();
    /** emitted loading data into an empty view
        @param dataObjects List of objects that are requested for visualization. */
    void resetStrategie(const QList<DataObject *> & dataObjects);

    void selectedDataChanged(RenderView * renderView, DataObject * dataObject);

    void beforeDeleteRenderedData(RenderedData * renderedData);

public slots:
    void render();

    void ShowInfo(const QStringList &info);

    void setStrategy(RenderViewStrategy * strategy);

protected:
    QWidget * contentWidget() override;
    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

private:
    void setupRenderer();
    void setupInteraction();
    void setInteractorStyle(const std::string & name);

    // data handling

    RenderedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    // remove some data objects from internal lists
    // @return list of dangling rendered data object that you have to delete.
    QList<RenderedData *> removeFromInternalLists(QList<DataObject *> dataObjects = {});

    // GUI / rendering tools

    RenderViewStrategy & strategy() const;
    
    void updateAxes();
    void createAxes();
    void setupColorMappingLegend();

private slots:
    /** update configuration widgets to focus on my content. */
    void updateGuiForContent();
    void updateGuiForSelectedData(RenderedData * renderedData);
    void updateGuiForRemovedData();
    /** scan all rendered data for changed attribute vectors */
    void fetchAllAttributeActors();

private:
    Ui_RenderView * m_ui;
    RenderViewStrategy * m_strategy;
    RenderViewStrategy * m_emptyStrategy;

    // rendered representations of data objects for this view
    QList<RenderedData *> m_renderedData;
    // objects that were loaded to the GPU but are currently not rendered 
    QList<RenderedData *> m_renderedDataCache;
    QMap<DataObject *, RenderedData *> m_dataObjectToRendered;
    QList<vtkActor *> m_attributeActors;

    double m_dataBounds[6];

    // Rendering components
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<PickingInteractorStyleSwitch> m_interactorStyle;
    vtkSmartPointer<vtkLightKit> m_lightKit;

    // visualization and annotation
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    bool m_axesEnabled;
    vtkScalarBarActor * m_colorMappingLegend;
    vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
    
    ScalarToColorMapping * m_scalarMapping;
};
