#pragma once

#include <map>
#include <vector>

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>
#include <core/utility/DataExtent.h>


class vtkCamera;
class vtkGenericOpenGLRenderWindow;
class vtkLightKit;
class vtkPropCollection;
class vtkRenderer;
class vtkRenderWindow;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkTextWidget;

class CameraInteractorStyleSwitch;
class GridAxes3DActor;
class PickerHighlighterInteractorObserver;
class RenderViewStrategy;
class RenderedData;
class RenderWindowCursorCallback;


/**
 * Base class for vtkRenderer based render views. 
 *
 * This class is NOT registered as a concrete RendererImplementation that can be used with the 
 * RendererImplementationSwitch. Instead, setup instances of this class manually or register subclasses of it 
 * (e.g., RendererImplementation3D).
 */
class GUI_API RendererImplementationBase3D : public RendererImplementation
{
public:
    explicit RendererImplementationBase3D(AbstractRenderView & renderView);
    ~RendererImplementationBase3D() override;

    ContentType contentType() const override;

    void applyCurrentCoordinateSystem(const CoordinateSystemSpecification & spec) override;

    bool canApplyTo(const QList<DataObject *> & data) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, 
        QList<DataObject *> & incompatibleObjects) override;

    std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject & dataObject) const override;

    void activate(t_QVTKWidget & qvtkWidget) override;
    void deactivate(t_QVTKWidget & qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void lookAtData(const VisualizationSelection & selection, unsigned int subViewIndex) override;
    void resetCamera(bool toInitialPosition, unsigned int subViewIndex) override;

    const DataBounds & dataBounds(unsigned int subViewIndex) const;

    void setAxesVisibility(bool visible) override;

    QList<RenderedData *> renderedData();

    CameraInteractorStyleSwitch * interactorStyleSwitch();

    vtkRenderWindow * renderWindow();
    vtkRenderer * renderer(unsigned int subViewIndex);
    vtkCamera * camera(unsigned int subViewIndex);

    /** Resets the clipping ranges of all sub-views with their specific cameras */
    void resetClippingRanges();

    vtkLightKit * lightKit();

    vtkTextWidget * titleWidget(unsigned int subViewIndex);

    GridAxes3DActor * axesActor(unsigned int subViewIndex);

    static vtkSmartPointer<GridAxes3DActor> createAxes();

protected:
    // per viewport objects
    struct ViewportSetup
    {
        vtkSmartPointer<vtkRenderer> renderer;

        // view props fetched per rendered data
        std::map<RenderedData *, vtkSmartPointer<vtkPropCollection>> dataProps;
        DataBounds dataBounds;

        vtkSmartPointer<GridAxes3DActor> axesActor;
        vtkSmartPointer<vtkTextWidget> titleWidget;
    };

protected:
    void onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    virtual void onDataVisibilityChanged(AbstractVisualizedData * content, unsigned int subViewIndex);

    void onSetSelection(const VisualizationSelection & selection) override;
    void onClearSelection() override;

    /** @return current strategy or, if not applicable, a null implementation */
    RenderViewStrategy & strategy() const;
    /** @return current strategy or, if not applicable, nullptr */
    virtual RenderViewStrategy * strategyIfEnabled() const = 0;
    ViewportSetup & viewportSetup(unsigned int subViewIndex = 0);

private:
    void initialize();
    /** update components that depend on the render window interactor */
    void assignInteractor();

    void updateAxes();
    void updateAxisLabelFormat(const CoordinateSystemSpecification & spec);
    void updateBounds();
    void addToBounds(RenderedData * renderedData, unsigned int subViewIndex);
    void removeFromBounds(RenderedData * renderedData, unsigned int subViewIndex);

    /** scan rendered data for changed attribute props (e.g., vectors) */
    void fetchViewProps(RenderedData * renderedData, unsigned int subViewIndex);

    void dataVisibilityChanged(RenderedData * renderedData, unsigned int subViewIndex);

private:
    bool m_isInitialized;
    std::unique_ptr<RenderViewStrategy> m_emptyStrategy;

    // -- setup --

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    std::unique_ptr<RenderWindowCursorCallback> m_cursorCallback;

    vtkSmartPointer<vtkLightKit> m_lightKit;

    vtkSmartPointer<CameraInteractorStyleSwitch> m_interactorStyle;
    vtkSmartPointer<PickerHighlighterInteractorObserver> m_pickerHighlighter;

    // -- contents and annotation --

    std::vector<ViewportSetup> m_viewportSetups;

private:
    Q_DISABLE_COPY(RendererImplementationBase3D)
};
