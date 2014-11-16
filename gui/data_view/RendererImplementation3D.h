#pragma once

#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkRenderer;
class vtkLightKit;
class vtkPropCollection;
class vtkCubeAxesActor;
class vtkScalarBarActor;
class vtkScalarBarWidget;

class PickingInteractorStyleSwitch;
class IPickingInteractorStyle;
class RenderViewStrategy;


class RendererImplementation3D : public RendererImplementation
{
    Q_OBJECT

public:
    RendererImplementation3D(RenderView & renderView, QObject * parent = nullptr);
    ~RendererImplementation3D() override;

    ContentType contentType() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

    void apply(QVTKWidget * qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void addRenderedData(RenderedData * renderedData) override;
    void removeRenderedData(RenderedData * renderedData) override;

    void highlightData(DataObject * dataObject, vtkIdType itemId = -1) override;
    virtual DataObject * highlightedData() override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;
    void resetCamera(bool toInitialPosition) override;

    const double * dataBounds() const;
    void dataBounds(double bounds[6]) const;

    void setAxesVisibility(bool visible) override;

    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;
    PickingInteractorStyleSwitch * interactorStyleSwitch();

    vtkLightKit * lightKit();
    vtkScalarBarWidget * colorLegendWidget();

    vtkCubeAxesActor * axesActor();

signals:
    void resetStrategy(const QList<DataObject *> & dataObjects);

public slots:
    void setStrategy(RenderViewStrategy * strategy);

private:
    void initialize();
    /** update components that depend on the render window interactor */
    void assignInteractor();

    void updateAxes();
    void createAxes();
    void setupColorMappingLegend();

    RenderViewStrategy & strategy() const;

private slots:
    /** scan rendered data for changed attribute props (e.g., vectors) */
    void fetchViewProps(RenderedData * renderedData);

    void dataVisibilityChanged(RenderedData * renderedData);

private:
    bool m_isInitialized;
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
    double m_dataBounds[6];

    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkScalarBarActor * m_colorMappingLegend;
    vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
};