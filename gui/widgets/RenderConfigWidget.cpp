#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

#include "core/Input.h"
#include "core/data_objects/DataObject.h"
#include "core/data_objects/RenderedData.h"


using namespace reflectionzeug;
using namespace propertyguizeug;


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_RenderConfigWidget())
, m_needsBrowserRebuild(true)
, m_propertyRoot(nullptr)
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
    updateWindowTitle();
    m_needsBrowserRebuild = true;
}

void RenderConfigWidget::setRenderedData(RenderedData * renderedData)
{
    updateWindowTitle(QString::fromStdString(renderedData->dataObject()->input()->name));

    m_propertyRoot = renderedData->configGroup();
    m_needsBrowserRebuild = true;
    emit repaint();
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
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->expandToDepth(0);
}
