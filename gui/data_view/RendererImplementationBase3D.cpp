#include "RendererImplementationBase3D.h"

#include <cassert>

#include <QMouseEvent>

#include <QVTKWidget.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <vtkCamera.h>
#include <vtkLightKit.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>

#include <vtkBoundingBox.h>
#include <vtkProperty2D.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkTextActor.h>
#include <vtkTextRepresentation.h>
#include <vtkTextWidget.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>
#include <gui/data_view/RenderViewStrategySwitch.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>
#include <gui/rendering_interaction/InteractorStyleImage.h>
#include <gui/data_view/RenderViewStrategyNull.h>


RendererImplementationBase3D::RendererImplementationBase3D(AbstractRenderView & renderView)
    : RendererImplementation(renderView)
    , m_strategy(nullptr)
    , m_isInitialized(false)
    , m_emptyStrategy(std::make_unique<RenderViewStrategyNull>(*this))
{
}

RendererImplementationBase3D::~RendererImplementationBase3D() = default;

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
        if (auto && rendered = obj->createRendered())
        {
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
    if (!m_renderView.isVisible())
        return;

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

    auto props = vtkSmartPointer<vtkPropCollection>::New();

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

    for (unsigned int i = 0; i < m_renderView.numberOfSubViews(); ++i)
    {
        colorMapping(i)->setVisualizedData(m_renderView.visualizations(i));
        if (interactorStyle())
            interactorStyle()->setRenderedData(renderedData());
    }
}

void RendererImplementationBase3D::setSelectedData(DataObject * dataObject, vtkIdType itemId)
{
    interactorStyle()->highlightIndex(dataObject, itemId);
}

DataObject * RendererImplementationBase3D::selectedData() const
{
    return m_interactorStyle->highlightedDataObject();
}

vtkIdType RendererImplementationBase3D::selectedIndex() const
{
    return m_interactorStyle->highlightedIndex();
}

void RendererImplementationBase3D::lookAtData(DataObject * dataObject, vtkIdType itemId, unsigned int subViewIndex)
{
    m_interactorStyle->SetCurrentRenderer(m_viewportSetups[subViewIndex].renderer);

    m_interactorStyle->lookAtIndex(dataObject, itemId);
}

void RendererImplementationBase3D::resetCamera(bool toInitialPosition, unsigned int subViewIndex)
{
    if (toInitialPosition)
        strategy().resetCamera(*camera(subViewIndex));

    auto & viewport = m_viewportSetups[subViewIndex];
    if (viewport.dataBounds.IsValid())
        viewport.renderer->ResetCamera();

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

vtkCamera * RendererImplementationBase3D::camera(unsigned int subViewIndex)
{
    return renderer(subViewIndex)->GetActiveCamera();
}

vtkLightKit * RendererImplementationBase3D::lightKit()
{
    return m_lightKit;
}

vtkTextWidget * RendererImplementationBase3D::titleWidget(unsigned int subViewIndex)
{
    return m_viewportSetups[subViewIndex].titleWidget;
}

ColorMapping * RendererImplementationBase3D::colorMapping(unsigned int subViewIndex)
{
    assert(m_viewportSetups[subViewIndex].colorMapping);
    return m_viewportSetups[subViewIndex].colorMapping;
}

vtkScalarBarWidget * RendererImplementationBase3D::colorLegendWidget(unsigned int subViewIndex)
{
    assert(m_viewportSetups[subViewIndex].scalarBarWidget);
    return m_viewportSetups[subViewIndex].scalarBarWidget;
}

vtkGridAxes3DActor * RendererImplementationBase3D::axesActor(unsigned int subViewIndex)
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

std::unique_ptr<AbstractVisualizedData> RendererImplementationBase3D::requestVisualization(DataObject & dataObject) const
{
    return dataObject.createRendered();
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

    m_viewportSetups.resize(renderView().numberOfSubViews());

    for (unsigned int subViewIndex = 0; subViewIndex < renderView().numberOfSubViews(); ++subViewIndex)
    {
        auto & viewport = m_viewportSetups[subViewIndex];

        auto renderer = vtkSmartPointer<vtkRenderer>::New();
        renderer->SetBackground(1, 1, 1);

        renderer->RemoveAllLights();
        m_lightKit->AddLightsToRenderer(renderer);

        viewport.renderer = renderer;
        m_renderWindow->AddRenderer(renderer);


        auto titleWidget = vtkSmartPointer<vtkTextWidget>::New();
        viewport.titleWidget = titleWidget;
        titleWidget->SetDefaultRenderer(viewport.renderer);
        titleWidget->SetCurrentRenderer(viewport.renderer);

        auto titleRepr = vtkSmartPointer<vtkTextRepresentation>::New();
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

        setupColorMapping(subViewIndex, viewport);
        
        viewport.axesActor = createAxes();
        renderer->AddViewProp(viewport.axesActor);
        renderer->AddViewProp(viewport.colorMappingLegend);
    }


    // -- interaction --

    m_interactorStyle = vtkSmartPointer<PickingInteractorStyleSwitch>::New();
    m_interactorStyle->SetCurrentRenderer(m_viewportSetups.front().renderer);

    m_interactorStyle->addStyle("InteractorStyle3D", vtkSmartPointer<InteractorStyle3D>::New());
    m_interactorStyle->addStyle("InteractorStyleImage", vtkSmartPointer<InteractorStyleImage>::New());

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::pointInfoSent,
        &m_renderView, &AbstractRenderView::ShowInfo);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked,
        this, &RendererImplementation::dataSelectionChanged);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::indexPicked,
        &m_renderView, &AbstractDataView::objectPicked);

    m_isInitialized = true;
}

void RendererImplementationBase3D::assignInteractor()
{
    for (auto & viewportSetup : m_viewportSetups)
    {
        viewportSetup.scalarBarWidget->SetInteractor(m_renderWindow->GetInteractor());
        viewportSetup.titleWidget->SetInteractor(m_renderWindow->GetInteractor());
        viewportSetup.titleWidget->On();
    }
}

void RendererImplementationBase3D::updateAxes()
{
    // TODO update only for relevant views

    for (auto & viewportSetup : m_viewportSetups)
    {

        // hide axes if we don't have visible objects
        if (!viewportSetup.dataBounds.IsValid())
        {
            viewportSetup.axesActor->VisibilityOff();
            continue;
        }

        viewportSetup.axesActor->SetVisibility(m_renderView.axesEnabled());

        double bounds[6];
        viewportSetup.dataBounds.GetBounds(bounds);
        viewportSetup.axesActor->SetGridBounds(bounds);

        viewportSetup.renderer->ResetCameraClippingRange();
    }
}

void RendererImplementationBase3D::updateBounds()
{
    // TODO update only for relevant views

    for (int viewportIndex = 0; viewportIndex < m_viewportSetups.size(); ++viewportIndex)
    {
        auto & dataBounds = m_viewportSetups[viewportIndex].dataBounds;

        dataBounds.Reset();

        for (AbstractVisualizedData * it : m_renderView.visualizations(viewportIndex))
            dataBounds.AddBounds(it->dataObject().bounds());

    }

    updateAxes();
}

void RendererImplementationBase3D::addToBounds(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto & dataBounds = m_viewportSetups[subViewIndex].dataBounds;

    dataBounds.AddBounds(renderedData->dataObject().bounds());

    // TODO update only if necessary
    updateAxes();
}

void RendererImplementationBase3D::removeFromBounds(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto & dataBounds = m_viewportSetups[subViewIndex].dataBounds;

    dataBounds.Reset();

    for (AbstractVisualizedData * it : m_renderView.visualizations(subViewIndex))
    {
        if (it == renderedData)
            continue;

        dataBounds.AddBounds(it->dataObject().bounds());
    }

    updateAxes();
}

vtkSmartPointer<vtkGridAxes3DActor> RendererImplementationBase3D::createAxes()
{
    double axesColor[3] = { 0, 0, 0 };
    double labelColor[3] = { 0, 0, 0 };

    auto gridAxes = vtkSmartPointer<vtkGridAxes3DActor>::New();
    gridAxes->SetFaceMask(0xFF);
    gridAxes->GenerateGridOn();
    gridAxes->GenerateEdgesOn();
    gridAxes->GenerateTicksOn();
    gridAxes->EnableLayerSupportOff();

    // we need to set a new vtkProperty object here, otherwise the changes will not apply to all faces/axes
    auto gridAxesProp = vtkSmartPointer<vtkProperty>::New();
    gridAxesProp->DeepCopy(gridAxes->GetProperty());

    gridAxesProp->BackfaceCullingOff();
    gridAxesProp->FrontfaceCullingOn();
    gridAxesProp->SetColor(axesColor);

    gridAxes->SetProperty(gridAxesProp);

    for (int i = 0; i < 3; ++i)
        gridAxes->GetLabelTextProperty(i)->SetColor(labelColor);
    
    // Will be shown when needed
    gridAxes->VisibilityOff();

    return gridAxes;
}

void RendererImplementationBase3D::setupColorMapping(unsigned int subViewIndex, ViewportSetup & viewportSetup)
{
    viewportSetup.colorMapping = colorMappingForSubView(subViewIndex);

    viewportSetup.colorMappingLegend = viewportSetup.colorMapping->colorMappingLegend();

    auto repr = vtkSmartPointer<vtkScalarBarRepresentation>::New();
    repr->SetScalarBarActor(viewportSetup.colorMappingLegend);

    viewportSetup.scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
    viewportSetup.scalarBarWidget->SetScalarBarActor(viewportSetup.colorMappingLegend);
    viewportSetup.scalarBarWidget->SetRepresentation(repr);
    viewportSetup.scalarBarWidget->EnabledOff();

    connect(viewportSetup.colorMapping, &ColorMapping::colorLegendVisibilityChanged,
        [&viewportSetup] (bool visible) 
    {
        if (visible)
        {
            // vtkAbstractWidget clears the current renderer when disabling and uses FindPokedRenderer while enabling
            // so ensure to always use the correct (not necessarily lastly clicked) renderer
            viewportSetup.scalarBarWidget->SetCurrentRenderer(viewportSetup.renderer);
        }
        viewportSetup.scalarBarWidget->SetEnabled(visible);
    });
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
        connect(&rendered->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationBase3D::updateBounds);
    }
    else
    {
        removeFromBounds(rendered, subViewIndex);
        disconnect(&rendered->dataObject(), &DataObject::boundsChanged, this, &RendererImplementationBase3D::updateBounds);
    }

    onDataVisibilityChanged(rendered, subViewIndex);
}
