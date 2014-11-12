#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/propertyguizeug_extension/PropertyEditorFactoryEx.h>
#include <gui/propertyguizeug_extension/PropertyPainterEx.h>
#include <gui/data_view/RenderView.h>


using namespace reflectionzeug;
using namespace propertyguizeug;


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RenderConfigWidget())
    , m_propertyBrowser(new PropertyBrowser(new PropertyEditorFactoryEx(), new PropertyPainterEx(), this))
    , m_propertyRoot(nullptr)
    , m_renderView(nullptr)
    , m_renderedData(nullptr)
{
    m_ui->setupUi(this);
    m_ui->content->layout()->addWidget(m_propertyBrowser);

    m_propertyBrowser->setAlwaysExpandGroups(true);

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

    m_propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    emit repaint();
}

void RenderConfigWidget::checkRemovedData(RenderedData * renderedData)
{
    if (m_renderedData == renderedData)
        clear();
}

void RenderConfigWidget::setCurrentRenderView(RenderView * renderView)
{
    clear();

    if (m_renderView)
        disconnect(m_renderView, &RenderView::beforeDeleteRenderedData, this, &RenderConfigWidget::checkRemovedData);

    m_renderView = renderView;
    m_renderedData = renderView ? renderView->highlightedRenderedData() : nullptr;
    if (renderView)
        connect(renderView, &RenderView::beforeDeleteRenderedData, this, &RenderConfigWidget::checkRemovedData);
    updateTitle();

    if (!m_renderedData)
        return;

    m_propertyRoot = m_renderedData->createConfigGroup();

    m_propertyBrowser->setRoot(m_propertyRoot);
    m_propertyBrowser->setColumnWidth(0, 135);

    emit repaint();
}

void RenderConfigWidget::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
        return;

    if (!m_renderView->dataObjects().contains(dataObject))
        return;

    RenderedData * renderedData = nullptr;
    for (RenderedData * rendered : m_renderView->renderedData())
    {
        if (rendered->dataObject() == dataObject)
        {
            renderedData = rendered;
            break;
        }
    }

    assert(renderedData);

    if (renderedData == m_renderedData)
        return;

    clear();

    m_renderedData = renderedData;
    updateTitle();
    m_propertyRoot = m_renderedData->createConfigGroup();
    m_propertyBrowser->setRoot(m_propertyRoot);
    m_propertyBrowser->setColumnWidth(0, 135);
    emit repaint();
}

void RenderConfigWidget::updateTitle()
{
    QString title;
    if (!m_renderedData)
        title = "(no object selected)";
    else
        title = QString::number(m_renderView->index()) + ": " + m_renderedData->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
