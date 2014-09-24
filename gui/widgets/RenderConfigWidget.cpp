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
    , m_rendererId(-1)
    , m_renderedData(nullptr)
{
    m_ui->setupUi(this);

    m_ui->propertyBrowser->setAlwaysExpandGroups(true);

    updateTitle();
}

RenderConfigWidget::~RenderConfigWidget()
{
    clear();
    delete m_ui;
}

void RenderConfigWidget::clear()
{
    m_renderedData = nullptr;

    updateTitle();

    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    emit repaint();
}

void RenderConfigWidget::setRenderedData(int rendererId, RenderedData * renderedData)
{
    clear();

    m_renderedData = renderedData;
    m_rendererId = rendererId;
    updateTitle();

    if (!renderedData)
        return;

    m_propertyRoot = renderedData->createConfigGroup();

    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->setColumnWidth(0, 135);

    emit repaint();
}

int RenderConfigWidget::rendererId() const
{
    return m_rendererId;
}

RenderedData * RenderConfigWidget::renderedData()
{
    return m_renderedData;
}

void RenderConfigWidget::updateTitle()
{
    QString title;
    if (!renderedData())
        title = "(no object selected)";
    else
        title = QString::number(m_rendererId) + ": " + renderedData()->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
