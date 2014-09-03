#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


using namespace reflectionzeug;
using namespace propertyguizeug;


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RenderConfigWidget())
    , m_propertyRoot(nullptr)
    , m_renderedData(nullptr)
{
    m_ui->setupUi(this);

    m_ui->propertyBrowser->setAlwaysExpandGroups(true);

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

    m_renderedData = renderedData;

    updateWindowTitle(renderedData);

    m_propertyRoot = renderedData->createConfigGroup();

    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->setColumnWidth(0, 135);

    emit repaint();
}

const RenderedData * RenderConfigWidget::renderedData() const
{
    return m_renderedData;
}

void RenderConfigWidget::updateWindowTitle(RenderedData * renderedData)
{
    const QString defaultTitle = "render configuration";

    if (!renderedData)
    {
        setWindowTitle(defaultTitle);
        return;
    }

    setWindowTitle(defaultTitle + ": " + renderedData->dataObject()->name());
}
