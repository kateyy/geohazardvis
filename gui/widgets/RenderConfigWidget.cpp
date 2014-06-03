#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>


using namespace reflectionzeug;
using namespace propertyguizeug;

namespace {
    enum class Interpolation { flat = VTK_FLAT, gouraud = VTK_GOURAUD, phong = VTK_PHONG };
    enum class Representation { points = VTK_POINTS, wireframe = VTK_WIREFRAME, surface = VTK_SURFACE };
}


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_RenderConfigWidget())
, m_needsBrowserRebuild(true)
, m_propertyRoot(nullptr)
, m_renderProperty(nullptr)
{
    m_ui->setupUi(this);

    updateWindowTitle();
}

RenderConfigWidget::~RenderConfigWidget()
{
    clear();
    delete m_ui;
}

void RenderConfigWidget::clear()
{
    setRenderProperty("", nullptr);
}

void RenderConfigWidget::setRenderProperty(QString propertyName, vtkProperty * renderProperty)
{
    updateWindowTitle(propertyName);

    m_renderProperty = renderProperty;
    m_needsBrowserRebuild = true;
    emit repaint();
}

void RenderConfigWidget::addPropertyGroup(reflectionzeug::PropertyGroup * group)
{
    m_addedGroups << group;
}

void RenderConfigWidget::paintEvent(QPaintEvent * event)
{
    if (m_needsBrowserRebuild)
    {
        m_needsBrowserRebuild = false;
        updatePropertyBrowser();
    }

    QDockWidget::paintEvent(event);
}

void RenderConfigWidget::updateWindowTitle(QString propertyName)
{
    const QString defaultTitle = "render configuration";

    if (propertyName.isEmpty())
    {
        setWindowTitle(defaultTitle);
        return;
    }

    setWindowTitle(defaultTitle + ": " + propertyName);
}

void RenderConfigWidget::updatePropertyBrowser()
{
    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;

    m_propertyRoot = new PropertyGroup("root");

    auto * renderSettings = new PropertyGroup("renderSettings");
    renderSettings->setTitle("rendering");
    m_propertyRoot->addProperty(renderSettings);

    if (m_renderProperty)
    {
        auto * color = renderSettings->addProperty<Color>("color",
            [this]() {
            double * color = m_renderProperty->GetColor();
            return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
        },
            [this](const Color & color) {
            m_renderProperty->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
            emit renderPropertyChanged();
        });


        auto * edgesVisible = renderSettings->addProperty<bool>("edgesVisible",
            [this]() {
            return (m_renderProperty->GetEdgeVisibility() == 0) ? false : true;
        },
            [this](bool vis) {
            m_renderProperty->SetEdgeVisibility(vis);
            emit renderPropertyChanged();
        });
        edgesVisible->setTitle("edge visible");

        auto * lineWidth = renderSettings->addProperty<float>("lineWidth",
            std::bind(&vtkProperty::GetLineWidth, m_renderProperty),
            [this](float width) {
            m_renderProperty->SetLineWidth(width);
            emit renderPropertyChanged();
        });
        lineWidth->setTitle("line width");
        lineWidth->setMinimum(0.1);
        lineWidth->setMaximum(std::numeric_limits<float>::max());
        lineWidth->setStep(0.1);

        auto * edgeColor = renderSettings->addProperty<Color>("edgeColor",
            [this]() {
            double * color = m_renderProperty->GetEdgeColor();
            return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
        },
            [this](const Color & color) {
            m_renderProperty->SetEdgeColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
            emit renderPropertyChanged();
        });
        edgeColor->setTitle("edge color");


        auto * representation = renderSettings->addProperty<Representation>("representation",
            [this]() {
            return static_cast<Representation>(m_renderProperty->GetRepresentation());
        },
            [this](const Representation & rep) {
            m_renderProperty->SetRepresentation(static_cast<int>(rep));
            emit renderPropertyChanged();
        });
        representation->setStrings({
            { Representation::points, "points" },
            { Representation::wireframe, "wireframe" },
            { Representation::surface, "surface" }
        });

        auto * lightingEnabled = renderSettings->addProperty<bool>("lightingEnabled",
            std::bind(&vtkProperty::GetLighting, m_renderProperty),
            [this](bool enabled) {
            m_renderProperty->SetLighting(enabled);
            emit renderPropertyChanged();
        });
        lightingEnabled->setTitle("lighting enabled");

        auto * interpolation = renderSettings->addProperty<Interpolation>("interpolation",
            [this]() {
            return static_cast<Interpolation>(m_renderProperty->GetInterpolation());
        },
            [this](const Interpolation & i) {
            m_renderProperty->SetInterpolation(static_cast<int>(i));
            emit renderPropertyChanged();
        });
        interpolation->setTitle("shading interpolation");
        interpolation->setStrings({
            { Interpolation::flat, "flat" },
            { Interpolation::gouraud, "gouraud" },
            { Interpolation::phong, "phong" }
        });

        auto transparency = renderSettings->addProperty<double>("transparency",
            [this]() {
            return 1.0 - m_renderProperty->GetOpacity();
        },
            [this](double transparency) {
            m_renderProperty->SetOpacity(1.0 - transparency);
            emit renderPropertyChanged();
        });
        transparency->setMinimum(0.f);
        transparency->setMaximum(1.f);
        transparency->setStep(0.01f);
    }

    //for (auto * group : m_addedGroups)
    //    m_propertyRoot->addProperty(group);

    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->expandToDepth(0);
}
