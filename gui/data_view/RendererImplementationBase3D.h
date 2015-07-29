#pragma once

#include <QVector>

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
    RendererImplementationBase3D(AbstractRenderView & renderView);
    ~RendererImplementationBase3D() override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & data) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, 
        QList<DataObject *> & incompatibleObjects) override;

    std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject & dataObject) const override;

    void activate(QVTKWidget * qvtkWidget) override;
    void deactivate(QVTKWidget * qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void setSelectedData(DataObject * dataObject, vtkIdType itemId = -1) override;
    DataObject * selectedData() const override;
    vtkIdType selectedIndex() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;
    void resetCamera(bool toInitialPosition) override;

    void dataBounds(double bounds[6], unsigned int subViewIndex = 0) const;

    void setAxesVisibility(bool visible) override;

    QList<RenderedData *> renderedData();

    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;
    PickingInteractorStyleSwitch * interactorStyleSwitch();

    vtkRenderWindow * renderWindow();
    vtkRenderer * renderer(unsigned int subViewIndex = 0);
    vtkCamera * camera();

    vtkLightKit * lightKit();

    vtkTextWidget * titleWidget(unsigned int subViewIndex);
    ColorMapping * colorMapping(unsigned int subViewIndex);
    vtkScalarBarWidget * colorLegendWidget(unsigned int subViewIndex);

    vtkCubeAxesActor * axesActor(unsigned int subViewIndex = 0);

    void setStrategy(RenderViewStrategy * strategy);

protected:
    // per viewport objects
    struct ViewportSetup
    {
        vtkSmartPointer<vtkRenderer> renderer;

        // view props fetched per rendered data
        QMap<RenderedData *, vtkSmartPointer<vtkPropCollection>> dataProps;
        vtkBoundingBox dataBounds;

        // Color mapping that is used for this sub-view. It may be shared with another view
        ColorMapping * colorMapping;
        vtkSmartPointer<vtkScalarBarWidget> scalarBarWidget;
        vtkScalarBarActor * colorMappingLegend;

        vtkSmartPointer<vtkCubeAxesActor> axesActor;
        vtkSmartPointer<vtkTextWidget> titleWidget;
    };

protected:
    bool eventFilter(QObject * watched, QEvent * event) override;

    void onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    virtual void onDataVisibilityChanged(AbstractVisualizedData * content, unsigned int subViewIndex);
    void onRenderViewVisualizationChanged() override;

    RenderViewStrategy & strategy() const;
    ViewportSetup & viewportSetup(unsigned int subViewIndex = 0);

    unsigned int subViewIndexAtPos(const QPoint pixelCoordinate) const;

    /** Provide a ColorMapping instance to be used with the specified sub-view.
      * The ownership of the ColorMapping will remain in sub-classes that implement this function. */
    virtual ColorMapping * colorMappingForSubView(unsigned int subViewIndex) = 0;

private:
    void initialize();
    /** update components that depend on the render window interactor */
    void assignInteractor();

    void updateAxes();
    void updateBounds();
    void addToBounds(RenderedData * renderedData, unsigned int subViewIndex);
    void removeFromBounds(RenderedData * renderedData, unsigned int subViewIndex);
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkCamera * camera);
    void setupColorMapping(unsigned int subViewIndex, ViewportSetup & viewportSetup);

    void updateActiveSubView(unsigned int subViewIndex);

private slots:
    /** scan rendered data for changed attribute props (e.g., vectors) */
    void fetchViewProps(RenderedData * renderedData, unsigned int subViewIndex);

    void dataVisibilityChanged(RenderedData * renderedData, unsigned int subViewIndex);

protected:
    RenderViewStrategy * m_strategy;

private:
    bool m_isInitialized;
    std::unique_ptr<RenderViewStrategy> m_emptyStrategy;

    // -- setup --
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;

    vtkSmartPointer<vtkLightKit> m_lightKit;

    vtkSmartPointer<PickingInteractorStyleSwitch> m_interactorStyle;

    // -- contents and annotation --
    QVector<ViewportSetup> m_viewportSetups;
};
