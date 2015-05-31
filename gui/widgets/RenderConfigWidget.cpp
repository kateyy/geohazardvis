#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/DataObject.h>
#include <core/AbstractVisualizedData.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


using namespace reflectionzeug;


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RenderConfigWidget())
    , m_propertyRoot(nullptr)
    , m_renderView(nullptr)
    , m_content(nullptr)
{
    m_ui->setupUi(this);

    m_ui->propertyBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->addPainterPlugin<ColorEditorRGB>();
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
    m_content = nullptr;

    updateTitle();

    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    update();
}

void RenderConfigWidget::checkDeletedContent(AbstractVisualizedData * content)
{
    if (m_content == content)
        clear();
}

void RenderConfigWidget::setCurrentRenderView(AbstractRenderView * renderView)
{
    clear();

    if (m_renderView)
    {
        disconnect(m_renderView, &AbstractRenderView::beforeDeleteVisualization,
            this, &RenderConfigWidget::checkDeletedContent);
        disconnect(m_renderView, &AbstractRenderView::selectedDataChanged,
            this, static_cast<void (RenderConfigWidget::*)(AbstractRenderView *, DataObject *)>(&RenderConfigWidget::setSelectedData));
    }

    m_renderView = renderView;
    m_content = renderView ? renderView->selectedDataVisualization() : nullptr;
    if (renderView)
    {
        connect(renderView, &AbstractRenderView::beforeDeleteVisualization,
            this, &RenderConfigWidget::checkDeletedContent);
        connect(m_renderView, &AbstractRenderView::selectedDataChanged,
            this, static_cast<void (RenderConfigWidget::*)(AbstractRenderView *, DataObject *)>(&RenderConfigWidget::setSelectedData));
    }
    updateTitle();

    if (!m_content)
        return;

    m_propertyRoot = m_content->createConfigGroup();

    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->setColumnWidth(0, 135);

    update();
}

void RenderConfigWidget::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
        return;

    AbstractVisualizedData * content = m_renderView->visualizationFor(dataObject);
    if (!content || (content == m_content))
        return;

    clear();

    m_content = content;
    updateTitle();
    m_propertyRoot = m_content->createConfigGroup();
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->setColumnWidth(0, 135);

    update();
}

void RenderConfigWidget::setSelectedData(AbstractRenderView * renderView, DataObject * dataObject)
{
    if (renderView != m_renderView)
        setCurrentRenderView(renderView);

    setSelectedData(dataObject);
}

void RenderConfigWidget::updateTitle()
{
    QString title;
    if (!m_content)
        title = "(No object selected)";
    else
        title = QString::number(m_renderView->index()) + ": " + m_content->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
