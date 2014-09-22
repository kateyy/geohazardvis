#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <vtkRenderer.h>
#include <vtkLightKit.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkcamerahelper.h>
#include "gui/data_view/RenderView.h"


using namespace reflectionzeug;
using namespace propertyguizeug;


RendererConfigWidget::RendererConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RendererConfigWidget())
    , m_propertyRoot(nullptr)
{
    m_ui->setupUi(this);

    m_ui->propertyBrowser->setAlwaysExpandGroups(true);

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

    for (RenderView * renderView : renderViews)
    {
        m_ui->relatedRenderer->addItem(
            renderView->friendlyName(),
            reinterpret_cast<size_t>(renderView));
    }

    if (renderViews.isEmpty())
        return;
}

void RendererConfigWidget::setCurrentRenderView(RenderView * renderView)
{
    m_ui->relatedRenderer->setCurrentText(renderView->friendlyName());
}

void RendererConfigWidget::setCurrentRenderView(int index)
{
    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    if (index < 0)
        return;

    RenderView * renderView = reinterpret_cast<RenderView *>(
        m_ui->relatedRenderer->itemData(index, Qt::UserRole).toULongLong());
    assert(renderView);

    m_propertyRoot = createPropertyGroup(renderView);
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
}

PropertyGroup * RendererConfigWidget::createPropertyGroup(RenderView * renderView)
{
    PropertyGroup * root = new PropertyGroup();

    auto backgroundColor = root->addProperty<Color>("backgroundColor",
        [renderView]() {
        double * color = renderView->renderer()->GetBackground();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderView](const Color & color) {
        renderView->renderer()->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        renderView->render();
    });
    backgroundColor->setOption("title", "background color");

    auto cameraGroup = root->addGroup("Camera");
    {
        /** default view:
            up (0, 0, 1)
            pos(0, 0, 0)
            foc(0, 0, 1)
            azimuth==0° direction: (1, 0, 0)
        */
        vtkCamera * camera = renderView->renderer()->GetActiveCamera();
        auto prop_azimuth = cameraGroup->addProperty<double>("azimuth",
            [camera]() {
            return Camera::getAzimuth(camera);
        },
            [renderView, camera](const double & azimuth) {
            Camera::setAzimuth(camera, azimuth);
            renderView->render();
        });
        prop_azimuth->setOption("minimum", 0);
        prop_azimuth->setOption("maximum", 360);
    }

    PropertyGroup * lightingGroup = root->addGroup("Lighting");

    auto prop_intensity = lightingGroup->addProperty<double>("intensity",
        [renderView]() { return renderView->lightKit()->GetKeyLightIntensity(); },
        [renderView](double value) {
        renderView->lightKit()->SetKeyLightIntensity(value);
        renderView->render();
    });
    prop_intensity->setOption("minimum", 0);
    prop_intensity->setOption("maximum", 3);
    prop_intensity->setOption("step", 0.05);

    return root;
}
