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
    , m_ui{ std::make_unique<Ui_RenderConfigWidget>() }
    , m_propertyRoot{ nullptr }
    , m_renderView{ nullptr }
    , m_content{ nullptr }
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
}

void RenderConfigWidget::clear()
{
    m_content = nullptr;

    updateTitle();

    m_ui->propertyBrowser->setRoot(nullptr);
    m_propertyRoot.reset();

    update();
}

void RenderConfigWidget::checkDeletedContent(const QList<AbstractVisualizedData *> & content)
{
    if (content.contains(m_content))
    {
        clear();
    }
}

void RenderConfigWidget::setCurrentRenderView(AbstractRenderView * renderView)
{
    clear();

    if (m_renderView)
    {
        disconnect(m_renderView, &AbstractRenderView::beforeDeleteVisualizations,
            this, &RenderConfigWidget::checkDeletedContent);
        disconnect(m_renderView, &AbstractRenderView::visualizationSelectionChanged,
            this, &RenderConfigWidget::setSelectedVisualization);
    }

    m_renderView = renderView;
    m_content = renderView ? renderView->visualzationSelection().visualization : nullptr;
    if (renderView)
    {
        connect(renderView, &AbstractRenderView::beforeDeleteVisualizations,
            this, &RenderConfigWidget::checkDeletedContent);
        connect(m_renderView, &AbstractRenderView::visualizationSelectionChanged,
            this, &RenderConfigWidget::setSelectedVisualization);
    }
    updateTitle();

    if (!m_content)
    {
        return;
    }

    m_propertyRoot = m_content->createConfigGroup();

    m_ui->propertyBrowser->setRoot(m_propertyRoot.get());
    m_ui->propertyBrowser->resizeColumnToContents(0);

    update();
}

void RenderConfigWidget::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
    {
        return;
    }

    auto vis = m_renderView->visualizationFor(dataObject);
    if (!vis)
    {
        return;
    }

    setSelectedVisualization(m_renderView, VisualizationSelection(vis));
}

void RenderConfigWidget::setSelectedVisualization(AbstractRenderView * renderView, const VisualizationSelection & selection)
{
    if (m_renderView != renderView)
    {
        setCurrentRenderView(renderView);
    }

    if (!m_renderView)
    {
        return;
    }

    if (m_content == selection.visualization)
    {
        return;
    }

    clear();

    if (!selection.visualization)
    {
        return;
    }

    m_content = selection.visualization;
    updateTitle();
    m_propertyRoot = m_content->createConfigGroup();
    m_ui->propertyBrowser->setRoot(m_propertyRoot.get());
    m_ui->propertyBrowser->resizeColumnToContents(0);

    update();
}

void RenderConfigWidget::updateTitle()
{
    const auto && title = !m_content
        ? "(No object selected)"
        : QString::number(m_renderView->index()) + ": " + m_content->dataObject().name();

    m_ui->relatedDataObject->setText(title);
}
