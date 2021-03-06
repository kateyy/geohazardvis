/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RendererImplementationBase3D.h"

#include <cassert>
#include <cmath>

#include <QMouseEvent>
#include <QDebug>

#include <vtkBoundingBox.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkIdTypeArray.h>
#include <vtkLightKit.h>
#include <vtkMapper2D.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
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
#include <core/utility/font.h>
#include <core/utility/GridAxes3DActor.h>
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

void RendererImplementationBase3D::applyCurrentCoordinateSystem(const CoordinateSystemSpecification & spec)
{
    const auto contents = renderView().visualizations();
    for (auto visualization : contents)
    {
        auto rendered = static_cast<RenderedData *>(visualization);
        rendered->setDefaultCoordinateSystem(spec);
    }

    updateAxisLabelFormat(spec);

    updateBounds();

    for (unsigned int i = 0; i < renderView().numberOfSubViews(); ++i)
    {
        resetCamera(true, i);
    }
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
    m_renderWindow->SetInteractor(qvtkWidget.GetInteractorBase());
    // pass my render window to the qvtkWidget
    qvtkWidget.SetRenderWindow(m_renderWindow);

    qvtkWidget.GetInteractorBase()->SetInteractorStyle(m_interactorStyle);

    assignInteractor();

    m_cursorCallback->setQWidget(&qvtkWidget);

    renderView().setInfoTextCallback([this] () -> QString
    {
        m_pickerHighlighter->requestPickedInfoUpdate();
        return m_pickerHighlighter->pickedInfo();
    });

    updateAxisLabelFormat(renderView().currentCoordinateSystem());
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

    assert(dataProps.find(renderedData) == dataProps.end());
    dataProps.emplace(renderedData, props);

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

    auto propsIt = dataProps.find(renderedData);
    assert(propsIt != dataProps.end());
    vtkSmartPointer<vtkPropCollection> props = std::move(propsIt->second);
    assert(props);
    dataProps.erase(propsIt);

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
        viewport.axesActor->SetVisibility(visible && !viewport.dataBounds.isEmpty());
    }
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

GridAxes3DActor * RendererImplementationBase3D::axesActor(unsigned int subViewIndex)
{
    return m_viewportSetups[subViewIndex].axesActor;
}

std::unique_ptr<AbstractVisualizedData> RendererImplementationBase3D::requestVisualization(DataObject & dataObject) const
{
    auto rendered = dataObject.createRendered();

    const auto & coordinateSystem = renderView().currentCoordinateSystem();
    if (coordinateSystem.isValid())
    {
        rendered->setDefaultCoordinateSystem(coordinateSystem);
    }

    return std::move(rendered);
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
        titleWidget->KeyPressActivationOff();

        struct DisableWidgetIfEmptyCallback
        {
            static void impl(vtkObject *, unsigned long, void * clientData, void *)
            {
                auto textWidget = reinterpret_cast<vtkTextWidget *>(clientData);
                const auto input = textWidget->GetTextActor()->GetInput();
                const bool empty = input == nullptr || input[0] == 0;
                textWidget->SetEnabled(!empty);
            }
        };
        auto callback = vtkSmartPointer<vtkCallbackCommand>::New();
        callback->SetClientData(titleWidget.GetPointer());
        callback->SetCallback(&DisableWidgetIfEmptyCallback::impl);
        titleWidget->GetTextActor()->AddObserver(vtkCommand::ModifiedEvent, callback);
        DisableWidgetIfEmptyCallback::impl(nullptr, 0, titleWidget.GetPointer(), nullptr);

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
    }

    m_pickerHighlighter->SetInteractor(m_renderWindow->GetInteractor());
}

void RendererImplementationBase3D::updateAxes()
{
    // TODO update only for relevant views

    for (auto & viewportSetup : m_viewportSetups)
    {
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

    setAxesVisibility(m_renderView.axesEnabled());

    resetClippingRanges();
}

void RendererImplementationBase3D::updateAxisLabelFormat(const CoordinateSystemSpecification & spec)
{
    static const auto degreeLabel = (QString("%g") + QChar(0x00B0)).toUtf8();

    const auto labelXY = spec.type == CoordinateSystemType::geographic
        ? degreeLabel   // latitude/longitude
        : "%g km";      // Northing/Easting
    const auto labelZ = spec.type == CoordinateSystemType::geographic
        ? ""            // Elevation unit: not known
        : "%g km";      // Elevation unit: assume same as horizontal coordinates

    for (unsigned i = 0; i < renderView().numberOfSubViews(); ++i)
    {
        auto axes = axesActor(i);
        axes->SetPrintfAxisLabelFormat(0, labelXY.data());
        axes->SetPrintfAxisLabelFormat(1, labelXY.data());
        axes->SetPrintfAxisLabelFormat(2, labelZ);
    }
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

    auto && dataSetBounds = renderedData->visibleBounds();
    if (dataBounds.contains(dataSetBounds))
    {
        return;
    }

    dataBounds.add(dataSetBounds);

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

vtkSmartPointer<GridAxes3DActor> RendererImplementationBase3D::createAxes()
{
    auto gridAxes = vtkSmartPointer<GridAxes3DActor>::New();

    for (int i = 0; i < 3; ++i)
    {
        FontHelper::configureTextProperty(*gridAxes->GetLabelTextProperty(i));
        FontHelper::configureTextProperty(*gridAxes->GetTitleTextProperty(i));
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

    auto propsIt = dataProps.find(renderedData);
    assert(propsIt != dataProps.end());
    const auto & props = propsIt->second;
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
