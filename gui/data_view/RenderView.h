#pragma once

#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include <core/scalar_mapping/ScalarToColorMapping.h>
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

class ScalarMappingChooser;
class VectorMappingChooser;
class PickingInteractorStyleSwitch;
class IPickingInteractorStyle;
class RenderConfigWidget;
class Ui_RenderView;
class RenderViewStrategy;


class GUI_API RenderView : public AbstractDataView
{
    Q_OBJECT

public:
    RenderView(
        int index,
        ScalarMappingChooser & scalarMappingChooser,
        VectorMappingChooser & vectorMappingChooser,
        RenderConfigWidget & renderConfigWidget,
        QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

    /** add data objects to the view or make already added objects visible again */
    void addDataObjects(QList<DataObject *> dataObjects);
    /** remove rendered representations of data objects, don't delete data and settings */
    void hideDataObjects(QList<DataObject *> dataObjects);
    /** check if the this objects is currently rendered */
    bool isVisible(DataObject * dataObject) const;
    /** remove rendered representations and all references to the data objects */
    void removeDataObjects(QList<DataObject *> dataObjects);
    QList<DataObject *> dataObjects() const;
    QList<const RenderedData *> renderedData() const;

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
    void renderedDataChanged(const QList<RenderedData *> & renderedData);
    /** emitted loading data into an empty view
        @param dataObjects List of objects that are requested for visualization. */
    void resetStrategie(const QList<DataObject *> & dataObjects);

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
    void removeFromInternalLists(QList<DataObject *> dataObjects = {});
    void clearInternalLists();

    // GUI / rendering tools

    RenderViewStrategy & strategy() const;
    
    void updateAxes();
    static vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);
    void setupColorMappingLegend();

    void warnIncompatibleObjects(QStringList incompatibleObjects);

private slots:
    /** Updates the RenderConfigWidget to reflect the data's render properties. */
    void updateGuiForData(RenderedData * renderedData);
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

    // Rendering components
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<PickingInteractorStyleSwitch> m_interactorStyle;
    vtkSmartPointer<vtkLightKit> m_lightKit;

    // visualization and annotation
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    bool m_axesEnabled;
    vtkScalarBarActor * m_colorMappingLegend;
    vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
    
    // configuration widgets
    ScalarMappingChooser & m_scalarMappingChooser;
    VectorMappingChooser & m_vectorMappingChooser;
    ScalarToColorMapping m_scalarMapping;
    RenderConfigWidget & m_renderConfigWidget;
};
