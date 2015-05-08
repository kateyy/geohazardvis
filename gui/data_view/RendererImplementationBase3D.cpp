#include "RendererImplementationBase3D.h"

#include <cassert>

#include <QVTKWidget.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <vtkCamera.h>
#include <vtkLightKit.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>

#include <vtkBoundingBox.h>
#include <vtkCubeAxesActor.h>
#include <vtkProperty2D.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkTextActor.h>
#include <vtkTextRepresentation.h>
#include <vtkTextWidget.h>

#include <core/types.h>
#include <core/utility/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/color_mapping/ColorMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>
#include <gui/data_view/RenderViewStrategySwitch.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>
#include <gui/rendering_interaction/InteractorStyleImage.h>
#include <gui/data_view/RenderViewStrategyNull.h>


RendererImplementationBase3D::RendererImplementationBase3D(
    AbstractRenderView & renderView,
    QObject * parent)
    : RendererImplementation(renderView, parent)
    , m_strategy(nullptr)
    , m_isInitialized(false)
    , m_emptyStrategy(new RenderViewStrategyNull(*this, this))
    , m_colorMapping(nullptr)
{
}

QString RendererImplementationBase3D::name() const
{
    return "Renderer 3D";
}

ContentType RendererImplementationBase3D::contentType() const
{
    if (strategy().contains3dData())
        return ContentType::Rendered3D;
    else
        return ContentType::Rendered2D;
}

bool RendererImplementationBase3D::canApplyTo(const QList<DataObject *> & data)
{
    for (DataObject * obj : data)
    {
        if (RenderedData * rendered = obj->createRendered())
        {
            delete rendered;
            return true;
        }
    }

    return false;
}

QList<DataObject *> RendererImplementationBase3D::filterCompatibleObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects)
{
    assert(m_strategy);
    return m_strategy->filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementationBase3D::activate(QVTKWidget * qvtkWidget)
{
    RendererImplementation::activate(qvtkWidget);

    initialize();

    // make sure to reuse the existing render window interactor
    m_renderWindow->SetInteractor(qvtkWidget->GetInteractor());
    // pass my render window to the qvtkWidget
    qvtkWidget->SetRenderWindow(m_renderWindow);

    m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);

    assignInteractor();
}

void RendererImplementationBase3D::deactivate(QVTKWidget * qvtkWidget)
{
    RendererImplementation::deactivate(qvtkWidget);

    // this is our render window, so remove it from the widget
    qvtkWidget->SetRenderWindow(nullptr);
    // remove out interactor style from the widget's interactor
    m_renderWindow->GetInteractor()->SetInteractorStyle(nullptr);
    // the interactor belongs to the widget, we should reference it anymore
    m_renderWindow->SetInteractor(nullptr);
}

void RendererImplementationBase3D::render()
{
    m_renderWindow->Render();
}

vtkRenderWindowInteractor * RendererImplementationBase3D::interactor()
{
    return m_renderWindow->GetInteractor();
}

void RendererImplementationBase3D::onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    assert(dynamic_cast<RenderedData *>(content));
    RenderedData * renderedData = static_cast<RenderedData *>(content);

    VTK_CREATE(vtkPropCollection, props);

    auto && renderer = this->renderer(subViewIndex);
    auto && dataProps = m_viewportSetups[subViewIndex].dataProps;

    vtkCollectionSimpleIterator it;
    renderedData->viewProps()->InitTraversal(it);
    while (vtkProp * prop = renderedData->viewProps()->GetNextProp(it))
    {
        props->AddItem(prop);
        renderer->AddViewProp(prop);
    }

    assert(!dataProps.contains(renderedData));
    dataProps.insert(renderedData, props);

    addConnectionForContent(content,
        connect(renderedData, &RenderedData::viewPropCollectionChanged,
        [this, renderedData, subViewIndex] () { fetchViewProps(renderedData, subViewIndex); }));

    addConnectionForContent(content,
        connect(renderedData, &RenderedData::visibilityChanged,
        [this, renderedData, subViewIndex] (bool) { dataVisibilityChanged(renderedData, subViewIndex); }));

    dataVisibilityChanged(renderedData, subViewIndex);
}

void RendererImplementationBase3D::onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    assert(dynamic_cast<RenderedData *>(content));
    RenderedData * renderedData = static_cast<RenderedData *>(content);

    auto && renderer = this->renderer(subViewIndex);
    auto && dataProps = m_viewportSetups[subViewIndex].dataProps;

    vtkSmartPointer<vtkPropCollection> props = dataProps.take(renderedData);
    assert(props);

    vtkCollectionSimpleIterator it;
    props->InitTraversal(it);
    while (vtkProp * prop = props->GetNextProp(it))
        renderer->RemoveViewProp(prop);

    removeFromBounds(renderedData, subViewIndex);

    renderer->ResetCamera();
}

void RendererImplementationBase3D::onDataVisibilityChanged(AbstractVisualizedData * /*content*/, unsigned int /*subViewIndex*/)
{
}

void RendererImplementationBase3D::onRenderViewVisualizationChanged()
{
    RendererImplementation::onRenderViewVisualizationChanged();

    colorMapping()->setVisualizedData(m_renderView.visualizations());
    if (interactorStyle())
        interactorStyle()->setRenderedData(renderedData());
}

void RendererImplementationBase3D::setSelectedData(DataObject * dataObject, vtkIdType itemId)
{
    interactorStyle()->highlightCell(dataObject, itemId);
}

DataObject * RendererImplementationBase3D::selectedData() const
{
    return m_interactorStyle->highlightedObject();
}

vtkIdType RendererImplementationBase3D::selectedIndex() const
{
    return m_interactorStyle->highlightedCell();
}

void RendererImplementationBase3D::lookAtData(DataObject * dataObject, vtkIdType itemId)
{
    m_interactorStyle->lookAtCell(dataObject, itemId);
}

void RendererImplementationBase3D::resetCamera(bool toInitialPosition)
{
    if (toInitialPosition)
        strategy().resetCamera(*camera());

    for (auto && viewport : m_viewportSetups)
    {
        if (viewport.dataBounds.IsValid())
            viewport.renderer->ResetCamera();
    }

    render();
}

void RendererImplementationBase3D::dataBounds(double bounds[6], unsigned int subViewIndex) const
{
    m_viewportSetups[subViewIndex].dataBounds.GetBounds(bounds);
}

void RendererImplementationBase3D::setAxesVisibility(bool visible)
{
    for (auto && viewport : m_viewportSetups)
        viewport.axesActor->SetVisibility(visible);

    render();
}

QList<RenderedData *> RendererImplementationBase3D::renderedData()
{
    QList<RenderedData *> renderedList;
    for (AbstractVisualizedData * vis : m_renderView.visualizations())
    {
        if (RenderedData * rendered = dynamic_cast<RenderedData *>(vis))
            renderedList << rendered;
    }

    return renderedList;
}

IPickingInteractorStyle * RendererImplementationBase3D::interactorStyle()
{
    return m_interactorStyle;
}

const IPickingInteractorStyle * RendererImplementationBase3D::interactorStyle() const
{
    return m_interactorStyle;
}

PickingInteractorStyleSwitch * RendererImplementationBase3D::interactorStyleSwitch()
{
    return m_interactorStyle;
}

vtkRenderWindow * RendererImplementationBase3D::renderWindow()
{
    assert(m_renderWindow);
    return m_renderWindow;
}

vtkRenderer * RendererImplementationBase3D::renderer(unsigned int subViewIndex)
{
    assert((unsigned)m_viewportSetups.size() > subViewIndex);
    assert(m_viewportSetups[subViewIndex].renderer);
    return m_viewportSetups[subViewIndex].renderer;
}

vtkCamera * RendererImplementationBase3D::camera()
{
    // one camera for all
    return renderer(0)->GetActiveCamera();
}

vtkLightKit * RendererImplementationBase3D::lightKit()
{
    return m_lightKit;
}

vtkTextWidget * RendererImplementationBase3D::titleWidget(unsigned int subViewIndex)
{
    return m_viewportSetups[subViewIndex].titleWidget;
}

ColorMapping * RendererImplementationBase3D::colorMapping()
{
    assert(m_colorMapping);
    return m_colorMapping;
}

vtkScalarBarWidget * RendererImplementationBase3D::colorLegendWidget()
{
    return m_scalarBarWidget;
}

vtkCubeAxesActor * RendererImplementationBase3D::axesActor(unsigned int subViewIndex)
{
    return m_viewportSetups[subViewIndex].axesActor;
}

void RendererImplementationBase3D::setStrategy(RenderViewStrategy * strategy)
{
    if (m_strategy)
        m_strategy->deactivate();

    m_strategy = strategy;

    if (m_strategy)
        m_strategy->activate();
}

AbstractVisualizedData * RendererImplementationBase3D::requestVisualization(DataObject * dataObject) const
{
    return dataObject->createRendered();
}

void RendererImplementationBase3D::initialize()
{
    if (m_isInitialized)
        return;

    // -- lights --

    m_lightKit = vtkSmartPointer<vtkLightKit>::New();
    m_lightKit->SetKeyLightIntensity(1.0);


    // -- render (window) --

    m_renderWindow = vtkSmartPointer<vtkRenderWindow>::New();

    m_colorMapping = new ColorMapping(this);
    setupColorMappingLegend();

    VTK_CREATE(vtkCamera, camera);

    m_viewportSetups.resize(renderView().numberOfSubViews());

    for (auto && viewport : m_viewportSetups)
    {
        VTK_CREATE(vtkRenderer, renderer);
        renderer->SetBackground(1, 1, 1);
        renderer->SetActiveCamera(camera);

        renderer->RemoveAllLights();
        m_lightKit->AddLightsToRenderer(renderer);

        viewport.renderer = renderer;
        m_renderWindow->AddRenderer(renderer);


        VTK_CREATE(vtkTextWidget, titleWidget);
        viewport.titleWidget = titleWidget;
        titleWidget->SetDefaultRenderer(viewport.renderer);
        titleWidget->SetCurrentRenderer(viewport.renderer);

        VTK_CREATE(vtkTextRepresentation, titleRepr);
        vtkTextActor * titleActor = titleRepr->GetTextActor();
        titleActor->SetInput(" ");
        titleActor->GetTextProperty()->SetColor(0, 0, 0);
        titleActor->GetTextProperty()->SetVerticalJustificationToTop();

        titleRepr->GetPositionCoordinate()->SetValue(0.2, .85);
        titleRepr->GetPosition2Coordinate()->SetValue(0.6, .10);
        titleRepr->GetBorderProperty()->SetColor(0.2, 0.2, 0.2);

        titleWidget->SetRepresentation(titleRepr);
        titleWidget->SetTextActor(titleActor);
        titleWidget->SelectableOff();
        
        viewport.axesActor = createAxes(camera);
        renderer->AddViewProp(viewport.axesActor);
        renderer->AddViewProp(m_colorMappingLegend);
    }


    // -- interaction --

    m_interactorStyle = vtkSmartPointer<PickingInteractorStyleSwitch>::New();
    m_interactorStyle->SetDefaultRenderer(m_viewportSetups.first().renderer);   // TODO

    m_interactorStyle->addStyle("InteractorStyle3D", vtkSmartPointer<InteractorStyle3D>::New());
    m_interactorStyle->addStyle("InteractorStyleImage", vtkSmartPointer<InteractorStyleImage>::New());

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::pointInfoSent,
        &m_renderView, &AbstractRenderView::ShowInfo);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked,
        this, &RendererImplementation::dataSelectionChanged);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::cellPicked,
        &m_renderView, &AbstractDataView::objectPicked);

    m_isInitialized = true;
}

void RendererImplementationBase3D::assignInteractor()
{
    m_scalarBarWidget->SetInteractor(m_renderWindow->GetInteractor());

    for (auto && viewport : m_viewportSetups)
    {
        viewport.titleWidget->SetInteractor(m_renderWindow->GetInteractor());
        viewport.titleWidget->On();
    }
}

void RendererImplementationBase3D::updateAxes()
{
    // TODO update only for relevant views

    for (auto && viewport : m_viewportSetups)
    {

        // hide axes if we don't have visible objects
        if (!viewport.dataBounds.IsValid())
        {
            viewport.axesActor->VisibilityOff();
            continue;
        }

        viewport.axesActor->SetVisibility(m_renderView.axesEnabled());

        double bounds[6];
        viewport.dataBounds.GetBounds(bounds);
        viewport.axesActor->SetBounds(bounds);
    }
}

void RendererImplementationBase3D::updateBounds()
{
    // TODO update only for relevant views

    for (auto && viewport : m_viewportSetups)
    {
        auto && dataBounds = viewport.dataBounds;

        dataBounds.Reset();

        for (AbstractVisualizedData * it : m_renderView.visualizations())
            dataBounds.AddBounds(it->dataObject()->bounds());

    }

    updateAxes();
}

void RendererImplementationBase3D::addToBounds(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto && dataBounds = m_viewportSetups[subViewIndex].dataBounds;

    dataBounds.AddBounds(renderedData->dataObject()->bounds());

    // TODO update only if necessary
    updateAxes();
}

void RendererImplementationBase3D::removeFromBounds(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto && dataBounds = m_viewportSetups[subViewIndex].dataBounds;

    dataBounds.Reset();

    for (AbstractVisualizedData * it : m_renderView.visualizations())
    {
        if (it == renderedData)
            continue;

        dataBounds.AddBounds(it->dataObject()->bounds());
    }

    updateAxes();
}

vtkSmartPointer<vtkCubeAxesActor> RendererImplementationBase3D::createAxes(vtkCamera * camera)
{
    VTK_CREATE(vtkCubeAxesActor, axesActor);
    axesActor->SetCamera(camera);
    axesActor->SetFlyModeToOuterEdges();
    axesActor->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);
    //m_axesActor->SetUseTextActor3D(true);
    axesActor->SetTickLocationToBoth();
    // fix strange rotation of z-labels
    axesActor->GetLabelTextProperty(2)->SetOrientation(90);

    double axesColor[3] = { 0, 0, 0 };
    double gridColor[3] = { 0.7, 0.7, 0.7 };

    axesActor->GetXAxesLinesProperty()->SetColor(axesColor);
    axesActor->GetYAxesLinesProperty()->SetColor(axesColor);
    axesActor->GetZAxesLinesProperty()->SetColor(axesColor);
    axesActor->GetXAxesGridlinesProperty()->SetColor(gridColor);
    axesActor->GetYAxesGridlinesProperty()->SetColor(gridColor);
    axesActor->GetZAxesGridlinesProperty()->SetColor(gridColor);

    for (int i = 0; i < 3; ++i)
    {
        axesActor->GetTitleTextProperty(i)->SetColor(axesColor);
        axesActor->GetLabelTextProperty(i)->SetColor(axesColor);
    }

    axesActor->XAxisMinorTickVisibilityOff();
    axesActor->YAxisMinorTickVisibilityOff();
    axesActor->ZAxisMinorTickVisibilityOff();

    axesActor->DrawXGridlinesOn();
    axesActor->DrawYGridlinesOn();
    axesActor->DrawZGridlinesOn();

    return axesActor;
}

void RendererImplementationBase3D::setupColorMappingLegend()
{
    m_colorMappingLegend = m_colorMapping->colorMappingLegend();
    m_colorMappingLegend->SetAnnotationTextScaling(false);
    m_colorMappingLegend->SetBarRatio(0.2);
    m_colorMappingLegend->SetNumberOfLabels(7);
    m_colorMappingLegend->SetDrawBackground(true);
    m_colorMappingLegend->GetBackgroundProperty()->SetColor(1, 1, 1);
    m_colorMappingLegend->SetDrawFrame(true);
    m_colorMappingLegend->GetFrameProperty()->SetColor(0, 0, 0);
    m_colorMappingLegend->SetVerticalTitleSeparation(5);
    m_colorMappingLegend->SetTextPad(3);

    vtkTextProperty * labelProp = m_colorMappingLegend->GetLabelTextProperty();
    labelProp->SetShadow(false);
    labelProp->SetColor(0, 0, 0);
    labelProp->SetBold(false);
    labelProp->SetItalic(false);

    vtkTextProperty * titleProp = m_colorMappingLegend->GetTitleTextProperty();
    titleProp->SetShadow(false);
    titleProp->SetColor(0, 0, 0);
    titleProp->SetBold(false);
    titleProp->SetItalic(false);

    VTK_CREATE(vtkScalarBarRepresentation, repr);
    repr->SetScalarBarActor(m_colorMappingLegend);

    m_scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
    m_scalarBarWidget->SetScalarBarActor(m_colorMappingLegend);
    m_scalarBarWidget->SetRepresentation(repr);
    m_scalarBarWidget->EnabledOff();

    connect(m_colorMapping, &ColorMapping::colorLegendVisibilityChanged,
        [this] (bool visible) { m_scalarBarWidget->SetEnabled(visible); });
}

RenderViewStrategy & RendererImplementationBase3D::strategy() const
{
    assert(m_emptyStrategy);
    if (m_strategy)
        return *m_strategy;

    return *m_emptyStrategy;
}

RendererImplementationBase3D::ViewportSetup & RendererImplementationBase3D::viewportSetup(unsigned int subViewIndex)
{
    assert(subViewIndex < unsigned(m_viewportSetups.size()));
    return m_viewportSetups[subViewIndex];
}

void RendererImplementationBase3D::fetchViewProps(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto && dataProps = m_viewportSetups[subViewIndex].dataProps;
    auto && renderer = m_viewportSetups[subViewIndex].renderer;

    vtkSmartPointer<vtkPropCollection> props = dataProps.value(renderedData);
    assert(props);

    // TODO add/remove changes only

    // remove all old props from the renderer
    vtkCollectionSimpleIterator it;
    props->InitTraversal(it);
    while (vtkProp * prop = props->GetNextProp(it))
        renderer->RemoveViewProp(prop);
    props->RemoveAllItems();

    // insert all new props

    renderedData->viewProps()->InitTraversal(it);
    while (vtkProp * prop = renderedData->viewProps()->GetNextProp(it))
    {
        props->AddItem(prop);
        renderer->AddViewProp(prop);
    }

    render();
}

void RendererImplementationBase3D::dataVisibilityChanged(RenderedData * rendered, unsigned int subViewIndex)
{
    assert(rendered);

    if (rendered->isVisible())
    {
        addToBounds(rendered, subViewIndex);
        connect(rendered->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationBase3D::updateBounds);
    }
    else
    {
        removeFromBounds(rendered, subViewIndex);
        disconnect(rendered->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationBase3D::updateBounds);
    }

    onDataVisibilityChanged(rendered, subViewIndex);
}
