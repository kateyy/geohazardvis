#pragma once

#include <QMap>

#include <vtkSmartPointer.h>
#include <vtkBoundingBox.h>

#include <gui/data_view/RendererImplementation.h>


class vtkCamera;
class vtkRenderer;
class vtkLightKit;
class vtkPropCollection;
class vtkCubeAxesActor;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkTextWidget;

class IPickingInteractorStyle;
class PickingInteractorStyleSwitch;
class RenderViewStrategy;
class RenderViewStrategySwitch;
class RenderedData;
class ColorMapping;


/** Base class for vtkRenderer based render views. 

This class is NOT registered as a concrete RendererImplementation that can be used with the 
RendererImplementationSwitch. Instead, setup instances of this class manually or register subclasses of it 
(e.g., RendererImplementation3D). 

This class requires a valid ColorMapping and vtkRenderWindow to be passed to the constructor.
*/
class RendererImplementationBase3D : public RendererImplementation
{
public:
    RendererImplementationBase3D(
        AbstractRenderView & renderView, 
        ColorMapping * colorMapping, 
        vtkRenderWindow * renderWindow,
        QObject * parent = nullptr);

    QString name() const override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & data) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, 
        QList<DataObject *> & incompatibleObjects) override;

    AbstractVisualizedData * requestVisualization(DataObject * dataObject) const override;

    void activate(QVTKWidget * qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void setSelectedData(DataObject * dataObject, vtkIdType itemId = -1) override;
    DataObject * selectedData() const override;
    vtkIdType selectedIndex() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;
    void resetCamera(bool toInitialPosition) override;

    void dataBounds(double bounds[6]) const;

    void setAxesVisibility(bool visible) override;

    QList<RenderedData *> renderedData();

    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;
    PickingInteractorStyleSwitch * interactorStyleSwitch();

    vtkRenderWindow * renderWindow();
    vtkRenderer * renderer();
    vtkCamera * camera();

    vtkLightKit * lightKit();

    vtkTextWidget * titleWidget();
    ColorMapping * colorMapping();
    vtkScalarBarWidget * colorLegendWidget();

    vtkCubeAxesActor * axesActor();

    void setStrategy(RenderViewStrategy * strategy);

protected:

    void onAddContent(AbstractVisualizedData * content) override;
    void onRemoveContent(AbstractVisualizedData * content) override;

    RenderViewStrategy & strategy() const;

private:
    void initialize();
    /** update components that depend on the render window interactor */
    void assignInteractor();

    void updateAxes();
    void updateBounds();
    void addToBounds(RenderedData * renderedData);
    void removeFromBounds(RenderedData * renderedData);
    void createAxes();
    void setupColorMappingLegend();

private slots:
    /** scan rendered data for changed attribute props (e.g., vectors) */
    void fetchViewProps(RenderedData * renderedData);

    void dataVisibilityChanged(RenderedData * renderedData);

protected:
    RenderViewStrategy * m_strategy;

private:
    bool m_isInitialized;
    RenderViewStrategy * m_emptyStrategy;

    // -- setup --
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;

    vtkSmartPointer<vtkLightKit> m_lightKit;

    vtkSmartPointer<PickingInteractorStyleSwitch> m_interactorStyle;


    // -- contents and annotation --

    // view props fetched per rendered data
    QMap<RenderedData *, vtkSmartPointer<vtkPropCollection>> m_dataProps;
    vtkBoundingBox m_dataBounds;

    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkSmartPointer<vtkTextWidget> m_titleWidget;
    ColorMapping * m_colorMapping;
    vtkScalarBarActor * m_colorMappingLegend;
    vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
};