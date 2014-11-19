#include "RendererImplementation3D.h"

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
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkProperty2D.h>

#include <core/types.h>
#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/scalar_mapping/ScalarToColorMapping.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RenderViewStrategy.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>
#include <gui/rendering_interaction/InteractorStyleImage.h>
#include <gui/data_view/RenderViewStrategyNull.h>


RendererImplementation3D::RendererImplementation3D(RenderView & renderView, QObject * parent)
    : RendererImplementation(renderView, parent)
    , m_isInitialized(false)
    , m_strategy(nullptr)
    , m_emptyStrategy(new RenderViewStrategyNull(*this))
{
    connect(renderView.scalarMapping(), &ScalarToColorMapping::colorLegendVisibilityChanged,
        [this] (bool visible) { m_scalarBarWidget->SetEnabled(visible); });

    // TODO hack: replace by impl switch
    connect(&renderView, &RenderView::resetImplementation, this, &RendererImplementation3D::resetStrategy);
}

RendererImplementation3D::~RendererImplementation3D()
{
    delete m_emptyStrategy;
}

ContentType RendererImplementation3D::contentType() const
{
    if (strategy().contains3dData())
        return ContentType::Rendered3D;
    else
        return ContentType::Rendered2D;
}

QList<DataObject *> RendererImplementation3D::filterCompatibleObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects) const
{
    return strategy().filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementation3D::apply(QVTKWidget * qvtkWidget)
{
    initialize();

    qvtkWidget->SetRenderWindow(m_renderWindow);
    m_renderWindow->SetInteractor(qvtkWidget->GetInteractor());

    m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);

    assignInteractor();
}

void RendererImplementation3D::render()
{
    m_renderWindow->Render();
}

vtkRenderWindowInteractor * RendererImplementation3D::interactor()
{
    return m_renderWindow->GetInteractor();
}

void RendererImplementation3D::addRenderedData(RenderedData * renderedData)
{
    VTK_CREATE(vtkPropCollection, props);

    vtkCollectionSimpleIterator it;
    renderedData->viewProps()->InitTraversal(it);
    while (vtkProp * prop = renderedData->viewProps()->GetNextProp(it))
    {
        props->AddItem(prop);
        m_renderer->AddViewProp(prop);
    }

    assert(!m_dataProps.contains(renderedData));
    m_dataProps.insert(renderedData, props);

    connect(renderedData, &RenderedData::viewPropCollectionChanged,
        [this, renderedData] () { fetchViewProps(renderedData); });

    connect(renderedData, &RenderedData::visibilityChanged,
        [this, renderedData] (bool) { dataVisibilityChanged(renderedData); });

    dataVisibilityChanged(renderedData);
}

void RendererImplementation3D::removeRenderedData(RenderedData * renderedData)
{
    vtkSmartPointer<vtkPropCollection> props = m_dataProps.take(renderedData);
    assert(props);

    vtkCollectionSimpleIterator it;
    props->InitTraversal(it);
    while (vtkProp * prop = props->GetNextProp(it))
        m_renderer->RemoveViewProp(prop);

    updateAxes();

    m_renderer->ResetCamera();

    if (m_renderView.renderedData().isEmpty())
        setStrategy(nullptr);
}

void RendererImplementation3D::highlightData(DataObject * dataObject, vtkIdType itemId)
{
    interactorStyle()->highlightCell(dataObject, itemId);
}

DataObject * RendererImplementation3D::highlightedData()
{
    return m_interactorStyle->highlightedObject();
}

void RendererImplementation3D::lookAtData(DataObject * dataObject, vtkIdType itemId)
{
    m_interactorStyle->lookAtCell(dataObject, itemId);
}

void RendererImplementation3D::resetCamera(bool toInitialPosition)
{
    if (toInitialPosition)
        strategy().resetCamera(*m_renderer->GetActiveCamera());

    m_renderer->ResetCamera();

    render();
}

void RendererImplementation3D::dataBounds(double bounds[6]) const
{
    m_dataBounds.GetBounds(bounds);
}

void RendererImplementation3D::setAxesVisibility(bool visible)
{
    m_axesActor->SetVisibility(visible);

    render();
}

IPickingInteractorStyle * RendererImplementation3D::interactorStyle()
{
    return m_interactorStyle;
}

const IPickingInteractorStyle * RendererImplementation3D::interactorStyle() const
{
    return m_interactorStyle;
}

PickingInteractorStyleSwitch * RendererImplementation3D::interactorStyleSwitch()
{
    return m_interactorStyle;
}

vtkRenderer * RendererImplementation3D::renderer()
{
    assert(m_renderer);
    return m_renderer;
}

vtkCamera * RendererImplementation3D::camera()
{
    assert(renderer());
    return renderer()->GetActiveCamera();
}

vtkLightKit * RendererImplementation3D::lightKit()
{
    return m_lightKit;
}

vtkScalarBarWidget * RendererImplementation3D::colorLegendWidget()
{
    return m_scalarBarWidget;
}

vtkCubeAxesActor * RendererImplementation3D::axesActor()
{
    return m_axesActor;
}

void RendererImplementation3D::setStrategy(RenderViewStrategy * strategy)
{
    if (m_strategy)
        m_strategy->deactivate();

    m_strategy = strategy;

    if (m_strategy)
        m_strategy->activate();
}

void RendererImplementation3D::initialize()
{
    if (m_isInitialized)
        return;

    // -- render (window) --

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(1, 1, 1);

    m_renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    //m_renderWindow->SetAAFrames(0);
    //m_renderWindow->SetMultiSamples(0);
    m_renderWindow->AddRenderer(m_renderer);


    // -- lights --

    m_renderer->RemoveAllLights();
    m_lightKit = vtkSmartPointer<vtkLightKit>::New();
    m_lightKit->AddLightsToRenderer(m_renderer);


    // -- interaction --

    m_interactorStyle = vtkSmartPointer<PickingInteractorStyleSwitch>::New();
    m_interactorStyle->SetDefaultRenderer(m_renderer);

    m_interactorStyle->addStyle("InteractorStyle3D", vtkSmartPointer<InteractorStyle3D>::New());
    m_interactorStyle->addStyle("InteractorStyleImage", vtkSmartPointer<InteractorStyleImage>::New());

    connect(&m_renderView, &RenderView::renderedDataChanged,
        [this] () { interactorStyle()->setRenderedData(m_renderView.renderedData()); });

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::pointInfoSent, &m_renderView, &RenderView::ShowInfo);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked, this, &RendererImplementation3D::dataSelectionChanged);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::cellPicked, &m_renderView, &AbstractDataView::objectPicked);

    createAxes();
    setupColorMappingLegend();

    m_isInitialized = true;
}

void RendererImplementation3D::assignInteractor()
{
    m_scalarBarWidget->SetInteractor(m_renderWindow->GetInteractor());
}

void RendererImplementation3D::updateAxes()
{
    // hide axes if we don't have visible objects
    if (!m_dataBounds.IsValid())
    {
        m_axesActor->VisibilityOff();
        return;
    }

    m_axesActor->SetVisibility(m_renderView.axesEnabled());

    double bounds[6];
    m_dataBounds.GetBounds(bounds);
    m_axesActor->SetBounds(bounds);
}

void RendererImplementation3D::updateBounds()
{
    m_dataBounds.Reset();

    for (RenderedData * it : m_renderView.renderedData())
        m_dataBounds.AddBounds(it->dataObject()->bounds());

    updateAxes();
}

void RendererImplementation3D::addToBounds(RenderedData * renderedData)
{
    m_dataBounds.AddBounds(renderedData->dataObject()->bounds());

    // TODO update only if necessary
    updateAxes();
}

void RendererImplementation3D::removeFromBounds(RenderedData * renderedData)
{
    m_dataBounds.Reset();

    for (RenderedData * it : m_renderView.renderedData())
    {
        if (it == renderedData)
            continue;

        m_dataBounds.AddBounds(it->dataObject()->bounds());
    }

    updateAxes();
}

void RendererImplementation3D::createAxes()
{
    m_axesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    m_axesActor->SetCamera(m_renderer->GetActiveCamera());
    m_axesActor->SetFlyModeToOuterEdges();
    m_axesActor->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);
    //m_axesActor->SetUseTextActor3D(true);
    m_axesActor->SetTickLocationToBoth();
    // fix strange rotation of z-labels
    m_axesActor->GetLabelTextProperty(2)->SetOrientation(90);

    double axesColor[3] = { 0, 0, 0 };
    double gridColor[3] = { 0.7, 0.7, 0.7 };

    m_axesActor->GetXAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetYAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetZAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetXAxesGridlinesProperty()->SetColor(gridColor);
    m_axesActor->GetYAxesGridlinesProperty()->SetColor(gridColor);
    m_axesActor->GetZAxesGridlinesProperty()->SetColor(gridColor);

    for (int i = 0; i < 3; ++i)
    {
        m_axesActor->GetTitleTextProperty(i)->SetColor(axesColor);
        m_axesActor->GetLabelTextProperty(i)->SetColor(axesColor);
    }

    m_axesActor->XAxisMinorTickVisibilityOff();
    m_axesActor->YAxisMinorTickVisibilityOff();
    m_axesActor->ZAxisMinorTickVisibilityOff();

    m_axesActor->DrawXGridlinesOn();
    m_axesActor->DrawYGridlinesOn();
    m_axesActor->DrawZGridlinesOn();

    m_renderer->AddViewProp(m_axesActor);
}

void RendererImplementation3D::setupColorMappingLegend()
{
    m_colorMappingLegend = m_renderView.scalarMapping()->colorMappingLegend();
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

    m_renderer->AddViewProp(m_colorMappingLegend);
}

RenderViewStrategy & RendererImplementation3D::strategy() const
{
    assert(m_emptyStrategy);
    if (m_strategy)
        return *m_strategy;

    return *m_emptyStrategy;
}

void RendererImplementation3D::fetchViewProps(RenderedData * renderedData)
{
    vtkSmartPointer<vtkPropCollection> props = m_dataProps.value(renderedData);
    assert(props);

    // TODO add/remove changes only

    // remove all old props from the renderer
    vtkCollectionSimpleIterator it;
    props->InitTraversal(it);
    while (vtkProp * prop = props->GetNextProp(it))
        m_renderer->RemoveViewProp(prop);
    props->RemoveAllItems();

    // insert all new props

    renderedData->viewProps()->InitTraversal(it);
    while (vtkProp * prop = renderedData->viewProps()->GetNextProp(it))
    {
        props->AddItem(prop);
        m_renderer->AddViewProp(prop);
    }

    render();
}

void RendererImplementation3D::dataVisibilityChanged(RenderedData * rendered)
{
    assert(rendered);

    if (rendered->isVisible())
    {
        addToBounds(rendered);
        connect(rendered->dataObject(), &DataObject::boundsChanged, this, &RendererImplementation3D::updateBounds);
    }
    else
    {
        removeFromBounds(rendered);
        disconnect(rendered->dataObject(), &DataObject::boundsChanged, this, &RendererImplementation3D::updateBounds);
    }

    // reset strategy if we are empty
    if (!m_dataBounds.IsValid())
        setStrategy(nullptr);
}
