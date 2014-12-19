#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <vtkRenderer.h>
#include <vtkLightKit.h>
#include <vtkBrush.h>
#include <vtkCamera.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkCubeAxesActor.h>
#include <vtkTextProperty.h>
#include <vtkEventQtSlotConnect.h>

#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

#include <core/types.h>
#include <core/vtkcamerahelper.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementation3D.h>
#include <gui/data_view/RendererImplementationPlot.h>
#include <gui/propertyguizeug_extension/PropertyEditorFactoryEx.h>
#include <gui/propertyguizeug_extension/PropertyPainterEx.h>


using namespace reflectionzeug;
using namespace propertyguizeug;

namespace
{
enum class CubeAxesTickLocation
{
    inside = VTK_TICKS_INSIDE,
    outside = VTK_TICKS_OUTSIDE,
    both = VTK_TICKS_BOTH
};
enum class ProjectionType
{
    parallel,
    perspective
};
enum class SelectionMode
{
    none = vtkContextScene::SELECTION_NONE,
    default_ = vtkContextScene::SELECTION_DEFAULT,
    addition = vtkContextScene::SELECTION_ADDITION,
    subtraction = vtkContextScene::SELECTION_SUBTRACTION,
    toggle = vtkContextScene::SELECTION_TOGGLE
};
}


RendererConfigWidget::RendererConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RendererConfigWidget())
    , m_propertyBrowser(new PropertyBrowser(new PropertyEditorFactoryEx(), new PropertyPainterEx(), this))
    , m_propertyRoot(nullptr)
    , m_currentRenderView(nullptr)
    , m_eventConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
    , m_eventEmitters(vtkSmartPointer<vtkCollection>::New())
{
    m_ui->setupUi(this);
    m_ui->mainLayout->addWidget(m_propertyBrowser);

    m_propertyBrowser->setAlwaysExpandGroups(true);

    connect(m_ui->relatedRenderer, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, static_cast<void(RendererConfigWidget::*)(int)>(&RendererConfigWidget::setCurrentRenderView));
}

RendererConfigWidget::~RendererConfigWidget()
{
    clear();
    delete m_ui;
}

void RendererConfigWidget::clear()
{
    m_ui->relatedRenderer->clear();

    m_currentRenderView = nullptr;
    m_eventConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New(); // recreate, discarding all previous connections
    m_eventEmitters->RemoveAllItems();

    setCurrentRenderView(-1);
}

void RendererConfigWidget::setRenderViews(const QList<RenderView *> & renderViews)
{
    clear();

    for (RenderView * renderView : renderViews)
    {
        m_ui->relatedRenderer->addItem(
            renderView->friendlyName(),
            reinterpret_cast<qulonglong>(renderView));

        connect(renderView, &RenderView::windowTitleChanged, this, &RendererConfigWidget::updateRenderViewTitle);
    }

    if (renderViews.isEmpty())
        return;
}

void RendererConfigWidget::setCurrentRenderView(RenderView * renderView)
{
    if (renderView)
        m_ui->relatedRenderer->setCurrentText(renderView->friendlyName());
}

void RendererConfigWidget::setCurrentRenderView(int index)
{
    m_propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    if (index < 0)
        return;

    RenderView * lastRenderView = m_currentRenderView;
    m_currentRenderView = reinterpret_cast<RenderView *>(
        m_ui->relatedRenderer->itemData(index, Qt::UserRole).toULongLong());
    assert(m_currentRenderView);

    m_propertyRoot = createPropertyGroup(m_currentRenderView);
    m_propertyBrowser->setRoot(m_propertyRoot);
    m_propertyBrowser->setColumnWidth(0, 135);


    RendererImplementation3D * impl3D;
    if (lastRenderView && (impl3D = dynamic_cast<RendererImplementation3D *>(&lastRenderView->implementation())))
    {
        m_eventConnect->Disconnect(impl3D->camera(), vtkCommand::ModifiedEvent, this, SLOT(readCameraStats(vtkObject *)), this);
        m_eventEmitters->RemoveItem(impl3D->camera());
    }
    if (m_currentRenderView && (impl3D = dynamic_cast<RendererImplementation3D *>(&m_currentRenderView->implementation())))
    {
        m_eventConnect->Connect(impl3D->camera(), vtkCommand::ModifiedEvent, this, SLOT(readCameraStats(vtkObject *)), this);
        m_eventEmitters->AddItem(impl3D->camera());
    }
}

void RendererConfigWidget::updateRenderViewTitle(const QString & newTitle)
{
    RenderView * renderView = dynamic_cast<RenderView *>(sender());
    assert(renderView);
    qulonglong ptr = reinterpret_cast<qulonglong>(renderView);

    for (int i = 0; i < m_ui->relatedRenderer->count(); ++i)
    {
        if (m_ui->relatedRenderer->itemData(i, Qt::UserRole).toULongLong() == ptr)
        {
            m_ui->relatedRenderer->setItemText(i, newTitle);
            setCurrentRenderView(i);
            return;
        }
    }
}

void RendererConfigWidget::readCameraStats(vtkObject * caller)
{
    assert(vtkCamera::SafeDownCast(caller));
    vtkCamera * camera = static_cast<vtkCamera *>(caller);
    assert(m_currentRenderView && dynamic_cast<RendererImplementation3D *>(&m_currentRenderView->implementation()));
    assert(static_cast<RendererImplementation3D *>(&m_currentRenderView->implementation())->camera() == camera);

    std::function<void(AbstractProperty &)> updateFunc = [&updateFunc] (AbstractProperty & property)
    {
        if (property.isCollection())
            property.asCollection()->forEach(updateFunc);
        if (property.isValue())
            property.asValue()->valueChanged();
    };

    PropertyGroup * cameraGroup = nullptr;
    if (m_propertyRoot->propertyExists("Camera"))
        cameraGroup = m_propertyRoot->group("Camera");

    if (cameraGroup)
        cameraGroup->forEach(updateFunc);

    // update axes text label rotations for terrain view
    if (m_currentRenderView->contentType() == ContentType::Rendered3D)
    {
        RendererImplementation3D & impl3D = static_cast<RendererImplementation3D &>(m_currentRenderView->implementation());
        double azimuth = TerrainCamera::getAzimuth(*camera);
        if (TerrainCamera::getVerticalElevation(*camera) < 0)
            azimuth *= -1;

        impl3D.axesActor()->GetLabelTextProperty(0)->SetOrientation(azimuth - 90);
        impl3D.axesActor()->GetLabelTextProperty(1)->SetOrientation(azimuth);
    }
}

PropertyGroup * RendererConfigWidget::createPropertyGroup(RenderView * renderView)
{
    switch (renderView->contentType())
    {
    case ContentType::Rendered2D:
    case ContentType::Rendered3D:
    {
        RendererImplementation3D * impl3D = dynamic_cast<RendererImplementation3D *>(&renderView->implementation());
        assert(impl3D);
        return createPropertyGroupRenderer(renderView, impl3D);
    }
    case ContentType::Context2D:
    {
        RendererImplementationPlot * implPlot = dynamic_cast<RendererImplementationPlot *>(&renderView->implementation());
        assert(implPlot);
        return createPropertyGroupPlot(renderView, implPlot);
    }
    default:
        return new PropertyGroup();
    }
}

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupRenderer(RenderView * renderView, RendererImplementation3D * impl)
{
    PropertyGroup * root = new PropertyGroup();
    vtkRenderer * renderer = impl->renderer();

    auto backgroundColor = root->addProperty<Color>("backgroundColor",
        [renderer]() {
        double * color = renderer->GetBackground();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderView, renderer] (const Color & color) {
        renderer->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        renderView->render();
    });
    backgroundColor->setOption("title", "Background Color");

    auto cameraGroup = root->addGroup("Camera");
    {
        vtkCamera & camera = *impl->camera();

        if (renderView->contains3dData())
        {
            std::string degreesSuffix = (QString(' ') + QChar(0xB0)).toStdString();

            auto prop_azimuth = cameraGroup->addProperty<double>("Azimuth",
                [&camera]() {
                return TerrainCamera::getAzimuth(camera);
            },
                [renderView, &camera, renderer] (const double & azimuth) {
                TerrainCamera::setAzimuth(camera, azimuth);
                renderer->ResetCamera();
                renderView->render();
            });
            prop_azimuth->setOption("minimum", std::numeric_limits<double>::lowest());
            prop_azimuth->setOption("suffix", degreesSuffix);

            auto prop_elevation = cameraGroup->addProperty<double>("Elevation",
                [&camera]() {
                return TerrainCamera::getVerticalElevation(camera);
            },
                [renderView, &camera, renderer] (const double & elevation) {
                TerrainCamera::setVerticalElevation(camera, elevation);
                renderer->ResetCamera();
                renderView->render();
            });
            prop_elevation->setOption("minimum", -89);
            prop_elevation->setOption("maximum", 89);
            prop_elevation->setOption("suffix", degreesSuffix);
        }

        auto prop_focalPoint = cameraGroup->addProperty<std::array<double, 3>>("focalPoint",
            [&camera](size_t component) {
            return camera.GetFocalPoint()[component];
        },
            [&camera, renderView](size_t component, const double & value) {
            double foc[3];
            camera.GetFocalPoint(foc);
            foc[component] = value;
            camera.SetFocalPoint(foc);
            renderView->render();
        });
        prop_focalPoint->setOption("title", "Focal Point");
        char title[2] {'x', 0};
        for (quint8 i = 0; i < 3; ++i)
        {
            prop_focalPoint->at(i)->setOption("title", std::string(title));
            ++title[0];
        }


        if (renderView->contains3dData())
        {
            auto prop_projectionType = cameraGroup->addProperty<ProjectionType>("Projection",
                [&camera] () {
                return camera.GetParallelProjection() != 0 ? ProjectionType::parallel : ProjectionType::perspective;
            },
                [&camera, renderView] (ProjectionType type) {
                camera.SetParallelProjection(type == ProjectionType::parallel);
                renderView->render();
            });
            prop_projectionType->setStrings({
                    { ProjectionType::parallel, "parallel" },
                    { ProjectionType::perspective, "perspective" }
            });
        }
    }

    if (renderView->contains3dData())
    {
        PropertyGroup * lightingGroup = root->addGroup("Lighting");

        auto prop_intensity = lightingGroup->addProperty<double>("Intensity",
            [renderView, impl]() { return impl->lightKit()->GetKeyLightIntensity(); },
            [renderView, impl] (double value) {
            impl->lightKit()->SetKeyLightIntensity(value);
            renderView->render();
        });
        prop_intensity->setOption("minimum", 0);
        prop_intensity->setOption("maximum", 3);
        prop_intensity->setOption("step", 0.05);
    }

    auto axesGroup = root->addGroup("Axes");
    {
        vtkCubeAxesActor * axes = impl->axesActor();
        axesGroup->addProperty<bool>("Visible",
            std::bind(&RenderView::axesEnabled, renderView),
            std::bind(&RenderView::setEnableAxes, renderView, std::placeholders::_1));

        axesGroup->addProperty<bool>("Labels",
            [axes] () { return axes->GetXAxisLabelVisibility() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetXAxisLabelVisibility(visible);
            axes->SetYAxisLabelVisibility(visible);
            axes->SetZAxisLabelVisibility(visible);
            renderView->render();
        });

        auto prop_gridVisible = axesGroup->addProperty<bool>("gridLines",
            [axes] () { return axes->GetDrawXGridlines() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetDrawXGridlines(visible);
            axes->SetDrawYGridlines(visible);
            axes->SetDrawZGridlines(visible);
            renderView->render();
        });
        prop_gridVisible->setOption("title", "Grid Lines");

        auto prop_innerGridVisible = axesGroup->addProperty<bool>("innerGridLines",
            [axes] () { return axes->GetDrawXInnerGridlines() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetDrawXInnerGridlines(visible);
            axes->SetDrawYInnerGridlines(visible);
            axes->SetDrawZInnerGridlines(visible);
            renderView->render();
        });
        prop_innerGridVisible->setOption("title", "Inner Grid Lines");

        auto prop_foregroundGridLines = axesGroup->addProperty<bool>("foregroundGridLines",
            [axes] () { return axes->GetGridLineLocation() == VTK_GRID_LINES_ALL; },
            [axes, renderView] (bool v) {
            axes->SetGridLineLocation(v
                ? VTK_GRID_LINES_ALL
                : VTK_GRID_LINES_FURTHEST);
            renderView->render();
        });
        prop_foregroundGridLines->setOption("title", "Foreground Grid Lines");

        axesGroup->addProperty<bool>("ticks",
            [axes] () { return axes->GetXAxisTickVisibility() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetXAxisTickVisibility(visible);
            axes->SetYAxisTickVisibility(visible);
            axes->SetZAxisTickVisibility(visible);
            renderView->render();
        });

        auto prop_tickLocation = axesGroup->addProperty<CubeAxesTickLocation>("tickLocation",
            [axes] () { return static_cast<CubeAxesTickLocation>(axes->GetTickLocation()); },
            [axes, renderView] (CubeAxesTickLocation mode) {
            axes->SetTickLocation(static_cast<int>(mode));
            renderView->render();
        });
        prop_tickLocation->setOption("title", "Tick Location");
        prop_tickLocation->setStrings({
                { CubeAxesTickLocation::both, "inside/outside" },
                { CubeAxesTickLocation::inside, "inside" },
                { CubeAxesTickLocation::outside, "outside" } }
        );

        auto prop_minorTicksVisible = axesGroup->addProperty<bool>("minorTicks",
            [axes] () { return axes->GetXAxisMinorTickVisibility() != 0; },
            [axes, renderView] (bool visible) {
            axes->SetXAxisMinorTickVisibility(visible);
            axes->SetYAxisMinorTickVisibility(visible);
            axes->SetZAxisMinorTickVisibility(visible);
            renderView->render();
        });
        prop_minorTicksVisible->setOption("title", "Minor Ticks");
    }

    return root;
}

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupPlot(RenderView * /*renderView*/, RendererImplementationPlot * impl)
{
    PropertyGroup * root = new PropertyGroup();

    root->addProperty<bool>("AxesAutoUpdate", impl, &RendererImplementationPlot::axesAutoUpdate, &RendererImplementationPlot::setAxesAutoUpdate)
        ->setOption("title", "Automatic Axes Update");

    root->addProperty<bool>("ChartLegend",
        [impl] () { return impl->chart()->GetShowLegend(); },
        [impl] (bool v) { 
        impl->chart()->SetShowLegend(v);
        impl->render();
    })
        ->setOption("title", "Chart Legend");

    //auto backgroundColor = root->addProperty<Color>("backgroundColor",
    //    [impl] () {
    //    unsigned char * color = impl->chart()->GetBackgroundBrush()->GetColor();
    //    return Color(color[0], color[1], color[2]);
    //},
    //    [impl] (const Color & color) {

    //    impl->chart()->GetBackgroundBrush()->SetColor(color.red(), color.green(), color.blue());
    //    impl->render();
    //});
    //backgroundColor->setOption("title", "Background Color");

    //auto prop_selectionMode = root->addProperty<SelectionMode>("SelectionMode",
    //    [impl] () { return static_cast<SelectionMode>(impl->chart()->GetSelectionMode()); },
    //    [impl] (SelectionMode mode) {
    //    impl->chart()->SetSelectionMode(static_cast<int>(mode));
    //    impl->render();
    //});
    //prop_selectionMode->setStrings({
    //        { SelectionMode::none, "none" },
    //        { SelectionMode::default_, "default" },
    //        { SelectionMode::addition, "addition" },
    //        { SelectionMode::subtraction, "subtraction" },
    //        { SelectionMode::toggle, "toggle" } });

    return root;
}
