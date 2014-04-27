#include "renderconfigwidget.h"

#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

using namespace reflectionzeug;
using namespace propertyguizeug;

RenderConfigWidget::RenderConfigWidget(QWidget * parent)
: QDockWidget(parent)
, m_propertyBrowser(new PropertyBrowser())
, m_propertyRoot(nullptr)
, m_renderProperty(nullptr)
{
    setWidget(m_propertyBrowser);
}

RenderConfigWidget::~RenderConfigWidget()
{
    clear();
    delete m_propertyBrowser;
}

void RenderConfigWidget::clear()
{
    setRenderProperty(nullptr);
}

void RenderConfigWidget::setRenderProperty(vtkProperty * property)
{
    m_renderProperty = property;

    updatePropertyBrowser();
}

void RenderConfigWidget::updatePropertyBrowser()
{
    m_propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;

    if (m_renderProperty == nullptr)
        return;

    vtkProperty * renderProperty = m_renderProperty;

    auto * renderSettings = new PropertyGroup("renderSettings");

    auto * faceColor = renderSettings->addProperty<Color>("color",
        [renderProperty]() {
        double * color = renderProperty->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderProperty](const Color & color) {
        renderProperty->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
    });

    m_propertyRoot = renderSettings;
    m_propertyBrowser->setRoot(renderSettings);
}
