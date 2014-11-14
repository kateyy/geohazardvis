#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <vtkRenderer.h>
#include <vtkLightKit.h>
#include <vtkCamera.h>
#include <vtkCubeAxesActor.h>
#include <vtkTextProperty.h>
#include <vtkEventQtSlotConnect.h>

#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

#include <core/vtkcamerahelper.h>
#include <gui/data_view/RenderView.h>
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
}


RendererConfigWidget::RendererConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RendererConfigWidget())
    , m_propertyBrowser(new PropertyBrowser(new PropertyEditorFactoryEx(), new PropertyPainterEx(), this))
    , m_propertyRoot(nullptr)
    , m_activeCamera(nullptr)
    , m_eventConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
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

    setCurrentRenderView(-1);
}

void RendererConfigWidget::setRenderViews(const QList<RenderView *> & renderViews)
{
    clear();

    //for (RenderView * renderView : renderViews)
    //{
    //    m_ui->relatedRenderer->addItem(
    //        renderView->friendlyName(),
    //        reinterpret_cast<qulonglong>(renderView));

    //    connect(renderView, &RenderView::windowTitleChanged, this, &RendererConfigWidget::updateRenderViewTitle);

    //    m_eventConnect->Connect(renderView->renderer()->GetActiveCamera(), vtkCommand::ModifiedEvent, this, SLOT(readCameraStats(vtkObject *)), this);
    //}

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

    RenderView * renderView = reinterpret_cast<RenderView *>(
        m_ui->relatedRenderer->itemData(index, Qt::UserRole).toULongLong());
    assert(renderView);

    m_propertyRoot = createPropertyGroup(renderView);
    m_propertyBrowser->setRoot(m_propertyRoot);
    m_propertyBrowser->setColumnWidth(0, 135);
    //m_activeCamera = renderView->renderer()->GetActiveCamera();
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
    if (camera != m_activeCamera)
        return;

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

    if (!cameraGroup)
        return;

    cameraGroup->forEach(updateFunc);
    
    //RenderView * renderView = reinterpret_cast<RenderView *>(
    //    m_ui->relatedRenderer->currentData(Qt::UserRole).toULongLong());
    //assert(renderView);

    //// update axes text label rotations for terrain view
    //double up[3];
    //camera->GetViewUp(up);
    //if (up[2] > up[0] + up[1])
    //{
    //    double azimuth = TerrainCamera::getAzimuth(*camera);
    //    if (TerrainCamera::getVerticalElevation(*camera) < 0)
    //        azimuth *= -1;

    //    renderView->axesActor()->GetLabelTextProperty(0)->SetOrientation(azimuth - 90);
    //    renderView->axesActor()->GetLabelTextProperty(1)->SetOrientation(azimuth);
    //}
}

PropertyGroup * RendererConfigWidget::createPropertyGroup(RenderView * renderView)
{
    PropertyGroup * root = new PropertyGroup();

    //auto backgroundColor = root->addProperty<Color>("backgroundColor",
    //    [renderView]() {
    //    double * color = renderView->renderer()->GetBackground();
    //    return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    //},
    //    [renderView](const Color & color) {
    //    renderView->renderer()->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
    //    renderView->render();
    //});
    //backgroundColor->setOption("title", "background color");

    //auto cameraGroup = root->addGroup("Camera");
    //{
    //    vtkCamera & camera = *renderView->renderer()->GetActiveCamera();

    //    if (renderView->contains3dData())
    //    {
    //        std::string degreesSuffix = (QString(' ') + QChar(0xB0)).toStdString();

    //        auto prop_azimuth = cameraGroup->addProperty<double>("azimuth",
    //            [&camera]() {
    //            return TerrainCamera::getAzimuth(camera);
    //        },
    //            [renderView, &camera](const double & azimuth) {
    //            TerrainCamera::setAzimuth(camera, azimuth);
    //            renderView->renderer()->ResetCamera();
    //            renderView->render();
    //        });
    //        prop_azimuth->setOption("minimum", std::numeric_limits<double>::lowest());
    //        prop_azimuth->setOption("suffix", degreesSuffix);

    //        auto prop_elevation = cameraGroup->addProperty<double>("elevation",
    //            [&camera]() {
    //            return TerrainCamera::getVerticalElevation(camera);
    //        },
    //            [renderView, &camera](const double & elevation) {
    //            TerrainCamera::setVerticalElevation(camera, elevation);
    //            renderView->renderer()->ResetCamera();
    //            renderView->render();
    //        });
    //        prop_elevation->setOption("minimum", -89);
    //        prop_elevation->setOption("maximum", 89);
    //        prop_elevation->setOption("suffix", degreesSuffix);
    //    }

    //    auto prop_focalPoint = cameraGroup->addProperty<std::array<double, 3>>("focalPoint",
    //        [&camera](size_t component) {
    //        return camera.GetFocalPoint()[component];
    //    },
    //        [&camera, renderView](size_t component, const double & value) {
    //        double foc[3];
    //        camera.GetFocalPoint(foc);
    //        foc[component] = value;
    //        camera.SetFocalPoint(foc);
    //        renderView->render();
    //    });
    //    prop_focalPoint->setOption("title", "focal point");
    //    char title[2] {'x', 0};
    //    for (quint8 i = 0; i < 3; ++i)
    //    {
    //        prop_focalPoint->at(i)->setOption("title", std::string(title));
    //        ++title[0];
    //    }
    //}

    //if (renderView->contains3dData())
    //{
    //    PropertyGroup * lightingGroup = root->addGroup("Lighting");

    //    auto prop_intensity = lightingGroup->addProperty<double>("intensity",
    //        [renderView]() { return renderView->lightKit()->GetKeyLightIntensity(); },
    //        [renderView](double value) {
    //        renderView->lightKit()->SetKeyLightIntensity(value);
    //        renderView->render();
    //    });
    //    prop_intensity->setOption("minimum", 0);
    //    prop_intensity->setOption("maximum", 3);
    //    prop_intensity->setOption("step", 0.05);
    //}

    //auto axesGroup = root->addGroup("Axes");
    //{
    //    vtkCubeAxesActor * axes = renderView->axesActor();
    //    axesGroup->addProperty<bool>("visible",
    //        std::bind(&RenderView::axesEnabled, renderView),
    //        std::bind(&RenderView::setEnableAxes, renderView, std::placeholders::_1));

    //    axesGroup->addProperty<bool>("labels",
    //        [axes] () { return axes->GetXAxisLabelVisibility() != 0; },
    //        [axes, renderView] (bool visible) {
    //        axes->SetXAxisLabelVisibility(visible);
    //        axes->SetYAxisLabelVisibility(visible);
    //        axes->SetZAxisLabelVisibility(visible);
    //        renderView->render();
    //    });

    //    auto prop_gridVisible = axesGroup->addProperty<bool>("gridLines",
    //        [axes] () { return axes->GetDrawXGridlines() != 0; },
    //        [axes, renderView] (bool visible) {
    //        axes->SetDrawXGridlines(visible);
    //        axes->SetDrawYGridlines(visible);
    //        axes->SetDrawZGridlines(visible);
    //        renderView->render();
    //    });
    //    prop_gridVisible->setOption("title", "grid lines");

    //    auto prop_innerGridVisible = axesGroup->addProperty<bool>("innerGridLines",
    //        [axes] () { return axes->GetDrawXInnerGridlines() != 0; },
    //        [axes, renderView] (bool visible) {
    //        axes->SetDrawXInnerGridlines(visible);
    //        axes->SetDrawYInnerGridlines(visible);
    //        axes->SetDrawZInnerGridlines(visible);
    //        renderView->render();
    //    });
    //    prop_innerGridVisible->setOption("title", "inner grid lines");

    //    auto prop_foregroundGridLines = axesGroup->addProperty<bool>("foregroundGridLines",
    //        [axes] () { return axes->GetGridLineLocation() == VTK_GRID_LINES_ALL; },
    //        [axes, renderView] (bool v) {
    //        axes->SetGridLineLocation(v
    //            ? VTK_GRID_LINES_ALL
    //            : VTK_GRID_LINES_FURTHEST);
    //        renderView->render();
    //    });
    //    prop_foregroundGridLines->setOption("title", "foreground grid lines");

    //    auto prop_ticksVisible = axesGroup->addProperty<bool>("ticks",
    //        [axes] () { return axes->GetXAxisTickVisibility() != 0; },
    //        [axes, renderView] (bool visible) {
    //        axes->SetXAxisTickVisibility(visible);
    //        axes->SetYAxisTickVisibility(visible);
    //        axes->SetZAxisTickVisibility(visible);
    //        renderView->render();
    //    });

    //    auto prop_tickLocation = axesGroup->addProperty<CubeAxesTickLocation>("tickLocation",
    //        [axes] () { return static_cast<CubeAxesTickLocation>(axes->GetTickLocation()); },
    //        [axes, renderView] (CubeAxesTickLocation mode) {
    //        axes->SetTickLocation(static_cast<int>(mode));
    //        renderView->render();
    //    });
    //    prop_tickLocation->setOption("title", "tick location");
    //    prop_tickLocation->setStrings({
    //            { CubeAxesTickLocation::both, "inside/outside" },
    //            { CubeAxesTickLocation::inside, "inside" },
    //            { CubeAxesTickLocation::outside, "outside" } }
    //    );

    //    auto prop_minorTicksVisible = axesGroup->addProperty<bool>("minorTicks",
    //        [axes] () { return axes->GetXAxisMinorTickVisibility() != 0; },
    //        [axes, renderView] (bool visible) {
    //        axes->SetXAxisMinorTickVisibility(visible);
    //        axes->SetYAxisMinorTickVisibility(visible);
    //        axes->SetZAxisMinorTickVisibility(visible);
    //        renderView->render();
    //    });
    //    prop_minorTicksVisible->setOption("title", "minor ticks");
    //}

    return root;
}
