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

    updateTitle();
}

RenderConfigWidget::~RenderConfigWidget()
{
    clear();
    delete m_ui;
}

void RenderConfigWidget::clear()
{
    updateTitle();

    m_renderedData = nullptr;

    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    emit repaint();
}

void RenderConfigWidget::setRenderedData(int rendererId, RenderedData * renderedData)
{
    clear();

    m_renderedData = renderedData;
    updateTitle(rendererId);

    if (!renderedData)
        return;

    m_propertyRoot = renderedData->createConfigGroup();

    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->setColumnWidth(0, 135);

    emit repaint();
}

const RenderedData * RenderConfigWidget::renderedData() const
{
    return m_renderedData;
}

void RenderConfigWidget::updateTitle(int rendererId)
{
    QString title;
    if (!renderedData())
        title = "(no object selected)";
    else
        title = QString::number(rendererId) + ": " + renderedData()->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
