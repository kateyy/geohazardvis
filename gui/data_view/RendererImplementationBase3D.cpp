#include "RendererImplementationBase3D.h"

#include <cassert>
#include <cmath>

#include <QMouseEvent>
#include <QDebug>

#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkIdTypeArray.h>
#include <vtkLightKit.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextRepresentation.h>
#include <vtkTextWidget.h>

#include <core/types.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <core/utility/font.h>
#include <core/utility/vtkcamerahelper.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>
#include <gui/data_view/RenderViewStrategyNull.h>
#include <gui/data_view/t_QVTKWidget.h>
#include <gui/rendering_interaction/CameraInteractorStyleSwitch.h>
#include <gui/rendering_interaction/Highlighter.h>
#include <gui/rendering_interaction/InteractorStyleImage.h>
#include <gui/rendering_interaction/InteractorStyleTerrain.h>
#include <gui/rendering_interaction/PickerHighlighterInteractorObserver.h>
#include <gui/rendering_interaction/RenderWindowCursorCallback.h>


RendererImplementationBase3D::RendererImplementationBase3D(AbstractRenderView & renderView)
    : RendererImplementation(renderView)
    , m_isInitialized{ false }
    , m_emptyStrategy{ std::make_unique<RenderViewStrategyNull>(*this) }
{
}

RendererImplementationBase3D::~RendererImplementationBase3D() = default;

ContentType RendererImplementationBase3D::contentType() const
{
    return strategy().contains3dData()
        ? ContentType::Rendered3D
        : ContentType::Rendered2D;
}

bool RendererImplementationBase3D::canApplyTo(const QList<DataObject *> & data)
{
    for (auto obj : data)
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
    return strategy().filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementationBase3D::activate(t_QVTKWidget & qvtkWidget)
{
    RendererImplementation::activate(qvtkWidget);

    initialize();

    // make sure to reuse the existing render window interactor
    m_renderWindow->SetInteractor(qvtkWidget.GetInteractor());
    // pass my render window to the qvtkWidget
    qvtkWidget.SetRenderWindow(m_renderWindow);

    m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);

    assignInteractor();

    m_cursorCallback->setQWidget(&qvtkWidget);

    renderView().setInfoTextCallback([this] () -> QString
    {
        m_pickerHighlighter->requestPickedInfoUpdate();
        return m_pickerHighlighter->pickedInfo();
    });
}

void RendererImplementationBase3D::deactivate(t_QVTKWidget & qvtkWidget)
{
    RendererImplementation::deactivate(qvtkWidget);

    m_cursorCallback->setQWidget(nullptr);

    // this is our render window, so remove it from the widget
    qvtkWidget.SetRenderWindow(nullptr);
    // remove out interactor style from the widget's interactor
    m_renderWindow->GetInteractor()->SetInteractorStyle(nullptr);
    // the interactor belongs to the widget, we should reference it anymore
    m_renderWindow->SetInteractor(nullptr);
}

void RendererImplementationBase3D::render()
{
    if (!m_renderView.isVisible())
    {
        return;
    }

    m_renderWindow->Render();
}

vtkRenderWindowInteractor * RendererImplementationBase3D::interactor()
{
    return m_renderWindow->GetInteractor();
}

void RendererImplementationBase3D::onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    assert(dynamic_cast<RenderedData *>(content));
    auto renderedData = static_cast<RenderedData *>(content);

    renderedData->colorMapping().colorBarRepresentation().setContext(
        m_renderWindow->GetInteractor(),
        renderer(subViewIndex));

    auto props = vtkSmartPointer<vtkPropCollection>::New();

    auto && renderer = this->renderer(subViewIndex);
    auto && dataProps = m_viewportSetups[subViewIndex].dataProps;

    vtkCollectionSimpleIterator it;
    renderedData->viewProps()->InitTraversal(it);
    while (auto prop = renderedData->viewProps()->GetNextProp(it))
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
    auto renderedData = static_cast<RenderedData *>(content);

    // update camera and axes. signal/slots are already disconnected for this object, so explicitly trigger the updates
    if (content->isVisible())
    {
        content->setVisible(false);
        dataVisibilityChanged(renderedData, subViewIndex);
    }
    else
    {
        assert(selection().visualization != content);
    }

    auto & renderer = *this->renderer(subViewIndex);
    auto & dataProps = m_viewportSetups[subViewIndex].dataProps;

    vtkSmartPointer<vtkPropCollection> props = dataProps.take(renderedData);
    assert(props);

    vtkCollectionSimpleIterator it;
    props->InitTraversal(it);
    while (auto prop = props->GetNextProp(it))
    {
        renderer.RemoveViewProp(prop);
    }
}

void RendererImplementationBase3D::onDataVisibilityChanged(AbstractVisualizedData * /*content*/, unsigned int /*subViewIndex*/)
{
}

void RendererImplementationBase3D::onSetSelection(const VisualizationSelection & selection)
{
    auto & highlighter = m_pickerHighlighter->highlighter();
    if (!selection.visualization)
    {
        highlighter.clearIndices();
        return;
    }

    auto subViewIndex = m_renderView.subViewContaining(*selection.visualization);
    assert(subViewIndex >= 0);

    highlighter.setRenderer(viewportSetup(subViewIndex).renderer);
    highlighter.setTarget(selection);
}

void RendererImplementationBase3D::onClearSelection()
{
    m_pickerHighlighter->highlighter().clearIndices();
}

void RendererImplementationBase3D::lookAtData(const VisualizationSelection & selection, unsigned int subViewIndex)
{
    m_interactorStyle->SetCurrentRenderer(m_viewportSetups[subViewIndex].renderer);

    m_interactorStyle->moveCameraTo(selection);
}

void RendererImplementationBase3D::resetCamera(bool toInitialPosition, unsigned int subViewIndex)
{
    if (subViewIndex >= m_viewportSetups.size())
    {
        return;
    }

    auto & viewport = m_viewportSetups[subViewIndex];

    if (toInitialPosition)
    {
        interactorStyleSwitch()->resetCameraToDefault(*viewport.renderer->GetActiveCamera());
    }

    if (!viewport.dataBounds.isEmpty())
    {
        viewport.renderer->ResetCamera();
    }

    render();
}

const DataBounds & RendererImplementationBase3D::dataBounds(unsigned int subViewIndex) const
{
    return m_viewportSetups[subViewIndex].dataBounds;
}

void RendererImplementationBase3D::setAxesVisibility(bool visible)
{
    for (auto && viewport : m_viewportSetups)
    {
        viewport.axesActor->SetVisibility(visible);
    }

    render();
}

QList<RenderedData *> RendererImplementationBase3D::renderedData()
{
    QList<RenderedData *> renderedList;
    for (auto vis : m_renderView.visualizations())
    {
        if (auto rendered = dynamic_cast<RenderedData *>(vis))
        {
            renderedList << rendered;
        }
    }

    return renderedList;
}

CameraInteractorStyleSwitch * RendererImplementationBase3D::interactorStyleSwitch()
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

void RendererImplementationBase3D::resetClippingRanges()
{
    for (auto && viewportSetup : m_viewportSetups)
    {
        viewportSetup.renderer->ResetCameraClippingRange();
    }

    auto & firstRenderer = *m_viewportSetups[0].renderer;
    auto & firstCamera = *firstRenderer.GetActiveCamera();

    /** work around strangely behaving clipping with parallel projection (probably bug in vtkCamera)
      * @see http://www.paraview.org/Bug/view.php?id=7823 */
    if (firstCamera.GetParallelProjection())
    {
        vtkBoundingBox wholeBounds;
        for (auto & viewportSetup : m_viewportSetups)
        {
            double bounds[6];
            viewportSetup.renderer->ComputeVisiblePropBounds(bounds);
            wholeBounds.AddBounds(bounds);
        }

        double maxDistance = 0;
        for (int i = 0; i < 6; ++i)
        {
            maxDistance = std::max(maxDistance, std::abs(wholeBounds.GetBound(i)));
        }

        TerrainCamera::setDistanceFromFocalPoint(firstCamera, maxDistance);

        firstRenderer.ResetCameraClippingRange();
    }
}

vtkLightKit * RendererImplementationBase3D::lightKit()
{
    return m_lightKit;
}

vtkTextWidget * RendererImplementationBase3D::titleWidget(unsigned int subViewIndex)
{
    return m_viewportSetups[subViewIndex].titleWidget;
}

vtkGridAxes3DActor * RendererImplementationBase3D::axesActor(unsigned int subViewIndex)
{
    return m_viewportSetups[subViewIndex].axesActor;
}

std::unique_ptr<AbstractVisualizedData> RendererImplementationBase3D::requestVisualization(DataObject & dataObject) const
{
    return dataObject.createRendered();
}

void RendererImplementationBase3D::initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    // -- lights --

    m_lightKit = vtkSmartPointer<vtkLightKit>::New();
    m_lightKit->SetKeyLightIntensity(1.0);


    // -- render (window) --

    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

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
        auto titleActor = titleRepr->GetTextActor();
        titleActor->GetTextProperty()->SetColor(0, 0, 0);
        titleActor->GetTextProperty()->SetVerticalJustificationToTop();
        FontHelper::configureTextProperty(*titleActor->GetTextProperty());

        titleRepr->GetPositionCoordinate()->SetValue(0.2, .85);
        titleRepr->GetPosition2Coordinate()->SetValue(0.6, .10);
        titleRepr->GetBorderProperty()->SetColor(0.2, 0.2, 0.2);

        titleWidget->SetRepresentation(titleRepr);
        titleWidget->SetTextActor(titleActor);
        titleWidget->SelectableOff();

        viewport.axesActor = createAxes();
        renderer->AddViewProp(viewport.axesActor);
    }

    // -- interaction --

    m_interactorStyle = vtkSmartPointer<CameraInteractorStyleSwitch>::New();
    m_interactorStyle->SetCurrentRenderer(m_viewportSetups.front().renderer);

    m_interactorStyle->addStyle("InteractorStyleTerrain", vtkSmartPointer<InteractorStyleTerrain>::New());
    m_interactorStyle->addStyle("InteractorStyleImage", vtkSmartPointer<InteractorStyleImage>::New());

    // correctly show axes and labels based on the interaction style and resulting camera setup
    m_interactorStyle->AddObserver(InteractorStyleSwitch::StyleChangedEvent,
        this, &RendererImplementationBase3D::updateAxes);


    m_pickerHighlighter = vtkSmartPointer<PickerHighlighterInteractorObserver>::New();

    connect(m_pickerHighlighter, &PickerHighlighterInteractorObserver::pickedInfoChanged,
        &m_renderView, &AbstractRenderView::showInfoText);
    connect(m_pickerHighlighter, &PickerHighlighterInteractorObserver::dataPicked,
        [this] (const VisualizationSelection & selection)
    {
        if (selection.isEmpty())
        {
            m_renderView.clearSelection();
        }
        else
        {
            m_renderView.setVisualizationSelection(selection);
        }
    });
    connect(m_pickerHighlighter, &PickerHighlighterInteractorObserver::geometryChanged,
        this, &RendererImplementation::render);

    m_cursorCallback = std::make_unique<RenderWindowCursorCallback>();
    m_cursorCallback->setRenderWindow(m_renderWindow);

    m_isInitialized = true;
}

void RendererImplementationBase3D::assignInteractor()
{
    for (auto & viewportSetup : m_viewportSetups)
    {
        viewportSetup.titleWidget->SetInteractor(m_renderWindow->GetInteractor());
        viewportSetup.titleWidget->On();
    }

    m_pickerHighlighter->SetInteractor(m_renderWindow->GetInteractor());
}

void RendererImplementationBase3D::updateAxes()
{
    // TODO update only for relevant views

    for (auto & viewportSetup : m_viewportSetups)
    {

        // hide axes if we don't have visible objects
        if (viewportSetup.dataBounds.isEmpty())
        {
            viewportSetup.axesActor->VisibilityOff();
            continue;
        }

        viewportSetup.axesActor->SetVisibility(m_renderView.axesEnabled());

        auto gridBounds = viewportSetup.dataBounds;

        // for polygonal data (deltaZ > 0) and parallel projection in image interaction:
        // correctly show /required/ axes and labels (XY-plane only)
        if (m_interactorStyle->currentStyleName() == "InteractorStyleImage")
        {
            const auto zShifted = gridBounds[5] + 0.00001;   // always show axes in front of the data
            gridBounds[4] = gridBounds[5] = zShifted;
        }

        viewportSetup.axesActor->SetGridBounds(gridBounds.data());
    }

    resetClippingRanges();
}

void RendererImplementationBase3D::updateBounds()
{
    // TODO update only for relevant views

    for (size_t viewportIndex = 0; viewportIndex < m_viewportSetups.size(); ++viewportIndex)
    {
        auto & dataBounds = m_viewportSetups[viewportIndex].dataBounds;

        dataBounds = {};

        for (auto visualization : m_renderView.visualizations(int(viewportIndex)))
        {
            assert(dynamic_cast<RenderedData *>(visualization));
            auto rendered = static_cast<RenderedData *>(visualization);
            dataBounds.add(rendered->visibleBounds());
        }
    }

    updateAxes();
}

void RendererImplementationBase3D::addToBounds(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto & dataBounds = m_viewportSetups[subViewIndex].dataBounds;

    dataBounds.add(renderedData->visibleBounds());

    // TODO update only if necessary
    updateAxes();
}

void RendererImplementationBase3D::removeFromBounds(RenderedData * renderedData, unsigned int subViewIndex)
{
    auto & dataBounds = m_viewportSetups[subViewIndex].dataBounds;

    dataBounds = {};

    for (auto visualization : m_renderView.visualizations(subViewIndex))
    {
        if (visualization == renderedData)
        {
            continue;
        }

        assert(dynamic_cast<RenderedData *>(visualization));
        auto rendered = static_cast<RenderedData *>(visualization);
        dataBounds.add(rendered->visibleBounds());
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
    {
        FontHelper::configureTextProperty(*gridAxes->GetLabelTextProperty(i));
        FontHelper::configureTextProperty(*gridAxes->GetTitleTextProperty(i));
        gridAxes->GetLabelTextProperty(i)->SetColor(labelColor);
        gridAxes->GetTitleTextProperty(i)->SetColor(labelColor);
    }

    // Will be shown when needed
    gridAxes->VisibilityOff();

    return gridAxes;
}

RenderViewStrategy & RendererImplementationBase3D::strategy() const
{
    assert(m_emptyStrategy);
    if (auto s = strategyIfEnabled())
    {
        return *s;
    }

    return *m_emptyStrategy;
}

RendererImplementationBase3D::ViewportSetup & RendererImplementationBase3D::viewportSetup(unsigned int subViewIndex)
{
    assert(subViewIndex < m_viewportSetups.size());
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
    while (auto prop = props->GetNextProp(it))
    {
        renderer->RemoveViewProp(prop);
    }
    props->RemoveAllItems();

    // insert all new props

    renderedData->viewProps()->InitTraversal(it);
    while (auto prop = renderedData->viewProps()->GetNextProp(it))
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

    // If the object is currently removed, it's not yet removed from the list
    const auto && currentObjects = renderView().dataObjects();
    const bool isEmpty = currentObjects.isEmpty() ||
        (!rendered->isVisible()
            && currentObjects.size() == 1
            && currentObjects.front() == &rendered->dataObject());

    m_pickerHighlighter->SetEnabled(!isEmpty);

    if (rendered->isVisible())
    {
        // If there isn't any selected target and rendered is visible, use rendered as the currently
        // active object. This is the one the user most recently worked with.
        if (!selection().visualization)
        {
            m_renderView.setVisualizationSelection(VisualizationSelection(rendered));
        }
    }
    else if (selection().visualization == rendered)
    {
        // If the current object is selected but not visible anymore, clear the selection
        m_renderView.clearSelection();
    }

    onDataVisibilityChanged(rendered, subViewIndex);

    renderer(subViewIndex)->ResetCamera();
}
