#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <algorithm>

#include <reflectionzeug/PropertyGroup.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/TemporalPipelineMediator.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


using namespace reflectionzeug;


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui{ std::make_unique<Ui_RenderConfigWidget>() }
    , m_temporalSelector{ std::make_unique<TemporalPipelineMediator>() }
    , m_propertyRoot{ nullptr }
    , m_renderView{ nullptr }
    , m_content{ nullptr }
{
    m_ui->setupUi(this);
    m_ui->timeSelectionWidget->hide();

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
    disconnect(m_ui->timeStepCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &RenderConfigWidget::setCurrentTimeStep);
    disconnect(m_ui->timeStepSlider, &QSlider::valueChanged,
        this, &RenderConfigWidget::setCurrentTimeStep);

    m_content = nullptr;
    m_temporalSelector->setVisualization(nullptr);

    m_ui->timeSelectionWidget->hide();
    m_ui->timeStepCombo->clear();
    m_ui->timeStepSlider->setRange(0, 0);

    updateTitle();

    m_ui->propertyBrowser->setRoot(nullptr);
    m_propertyRoot.reset();

    update();
}

void RenderConfigWidget::setCurrentTimeStep(int timeStepIndex)
{
    const auto comboBlocker = QSignalBlocker(m_ui->timeStepCombo);
    const auto sliderBlocker = QSignalBlocker(m_ui->timeStepSlider);
    m_ui->timeStepCombo->setCurrentIndex(timeStepIndex);
    m_ui->timeStepSlider->setSliderPosition(timeStepIndex);

    m_temporalSelector->selectTimeStepByIndex(static_cast<size_t>(timeStepIndex));
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

    m_temporalSelector->setVisualization(m_content);
    const bool hasTemporalData = !m_temporalSelector->timeSteps().empty();
    m_ui->timeSelectionWidget->setVisible(hasTemporalData);
    if (hasTemporalData)
    {
        const auto & timeSteps = m_temporalSelector->timeSteps();
        for (auto && ts : timeSteps)
        {
            m_ui->timeStepCombo->addItem(QString::number(ts));
        }
        m_ui->timeStepSlider->setRange(0, static_cast<int>(timeSteps.size() - 1));

        m_ui->timeStepCombo->setCurrentIndex(static_cast<int>(m_temporalSelector->currentTimeStepIndex()));
        m_ui->timeStepSlider->setSliderPosition(static_cast<int>(m_temporalSelector->currentTimeStepIndex()));

        connect(m_ui->timeStepCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &RenderConfigWidget::setCurrentTimeStep);
        connect(m_ui->timeStepSlider, &QSlider::valueChanged,
            this, &RenderConfigWidget::setCurrentTimeStep);
    }

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
