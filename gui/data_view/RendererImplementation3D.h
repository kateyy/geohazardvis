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


class RendererImplementation3D : public RendererImplementation
{
    Q_OBJECT

public:
    RendererImplementation3D(RenderView & renderView, QObject * parent = nullptr);
    ~RendererImplementation3D() override;

    QString name() const override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & data) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget * qvtkWidget) override;
    void deactivate(QVTKWidget * qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void highlightData(DataObject * dataObject, vtkIdType itemId = -1) override;
    virtual DataObject * highlightedData() override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;
    void resetCamera(bool toInitialPosition) override;

    void dataBounds(double bounds[6]) const;

    void setAxesVisibility(bool visible) override;

    QList<RenderedData *> renderedData();

    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;
    PickingInteractorStyleSwitch * interactorStyleSwitch();

    vtkRenderer * renderer();
    vtkCamera * camera();

    vtkLightKit * lightKit();

    vtkTextWidget * titleWidget();
    ColorMapping * colorMapping();
    vtkScalarBarWidget * colorLegendWidget();

    vtkCubeAxesActor * axesActor();

signals:
    void resetStrategy(const QList<DataObject *> & dataObjects);

public slots:
    void setStrategy(RenderViewStrategy * strategy);

protected:
    AbstractVisualizedData * requestVisualization(DataObject * dataObject) const override;

    void onAddContent(AbstractVisualizedData * content) override;
    void onRemoveContent(AbstractVisualizedData * content) override;

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

    RenderViewStrategy & strategy() const;

private slots:
    /** scan rendered data for changed attribute props (e.g., vectors) */
    void fetchViewProps(RenderedData * renderedData);

    void dataVisibilityChanged(RenderedData * renderedData);
    void redirectRenderViewContentChanged();

private:
    bool m_isInitialized;
    RenderViewStrategySwitch * m_strategySwitch;
    RenderViewStrategy * m_strategy;
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

    static bool s_isRegistered;
};