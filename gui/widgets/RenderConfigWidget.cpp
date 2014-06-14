#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

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

    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    emit repaint();
}

void RenderConfigWidget::setRenderedData(RenderedData * renderedData)
{
    clear();

    if (!renderedData)
        return;

    updateWindowTitle(renderedData);

    m_propertyRoot = renderedData->createConfigGroup();

    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->expandToDepth(0);

    emit repaint();
}

void RenderConfigWidget::updateWindowTitle(RenderedData * renderedData)
{
    const QString defaultTitle = "render configuration";

    if (!renderedData)
    {
        setWindowTitle(defaultTitle);
        return;
    }

    setWindowTitle(defaultTitle + ": " + QString::fromStdString(renderedData->dataObject()->input()->name));
}
