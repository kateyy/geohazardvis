#include "RenderPropertyConfigWidget.h"
#include "ui_RenderPropertyConfigWidget.h"

#include <algorithm>
#include <limits>

#include <reflectionzeug/PropertyGroup.h>

#include <core/AbstractVisualizedData.h>
#include <core/TemporalPipelineMediator.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/qthelper.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


using namespace reflectionzeug;


RenderPropertyConfigWidget::RenderPropertyConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui{ std::make_unique<Ui_RenderPropertyConfigWidget>() }
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

RenderPropertyConfigWidget::~RenderPropertyConfigWidget()
{
    clear();
}

void RenderPropertyConfigWidget::clear()
{
    disconnectAll(m_guiConnections);

    m_content = nullptr;
    m_temporalSelector->setVisualization(nullptr);

    m_ui->timeSelectionWidget->hide();
    m_ui->startDateCombo->clear();
    m_ui->endDateCombo->clear();
    m_ui->startDateSlider->setRange(0, 0);
    m_ui->endDateSlider->setRange(0, 0);

    updateTitle();

    m_ui->propertyBrowser->setRoot(nullptr);
    m_propertyRoot.reset();

    update();
}

void RenderPropertyConfigWidget::setStartDate(int timeStepIndex)
{
    const auto comboBlocker = QSignalBlocker(m_ui->startDateCombo);
    const auto sliderBlocker = QSignalBlocker(m_ui->startDateSlider);
    m_ui->startDateCombo->setCurrentIndex(timeStepIndex);
    m_ui->startDateSlider->setSliderPosition(timeStepIndex);

    m_temporalSelector->selectTemporalDifferenceByIndex(
        static_cast<size_t>(m_ui->startDateCombo->currentIndex()),
        static_cast<size_t>(m_ui->endDateCombo->currentIndex()));
}

void RenderPropertyConfigWidget::setEndDate(int timeStepIndex)
{
    const auto comboBlocker = QSignalBlocker(m_ui->endDateCombo);
    const auto sliderBlocker = QSignalBlocker(m_ui->endDateSlider);
    m_ui->endDateCombo->setCurrentIndex(timeStepIndex);
    m_ui->endDateSlider->setSliderPosition(timeStepIndex);

    m_temporalSelector->selectTemporalDifferenceByIndex(
        static_cast<size_t>(m_ui->startDateCombo->currentIndex()),
        static_cast<size_t>(m_ui->endDateCombo->currentIndex()));
}

void RenderPropertyConfigWidget::checkDeletedContent(const QList<AbstractVisualizedData *> & content)
{
    if (content.contains(m_content))
    {
        clear();
    }
}

void RenderPropertyConfigWidget::setCurrentRenderView(AbstractRenderView * renderView)
{
    if (m_renderView)
    {
        disconnect(m_renderView, &AbstractRenderView::beforeDeleteVisualizations,
            this, &RenderPropertyConfigWidget::checkDeletedContent);
        disconnect(m_renderView, &AbstractRenderView::visualizationSelectionChanged,
            this, &RenderPropertyConfigWidget::setSelectedVisualization);
    }

    m_renderView = renderView;

    if (!m_renderView)
    {
        clear();
        return;
    }

    connect(renderView, &AbstractRenderView::beforeDeleteVisualizations,
        this, &RenderPropertyConfigWidget::checkDeletedContent);
    connect(m_renderView, &AbstractRenderView::visualizationSelectionChanged,
        this, &RenderPropertyConfigWidget::setSelectedVisualization);

    setSelectedVisualization(m_renderView, renderView->visualzationSelection());
}

void RenderPropertyConfigWidget::setSelectedData(DataObject * dataObject)
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

void RenderPropertyConfigWidget::setSelectedVisualization(AbstractRenderView * renderView, const VisualizationSelection & selection)
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
        const size_t countSizeT = std::min(
            static_cast<size_t>(std::numeric_limits<int>::max()),
            timeSteps.size());
        const int count = static_cast<int>(countSizeT);

        const auto currentSelectionSizeT = m_temporalSelector->differenceTimeStepIndices();
        const auto currentSelection = std::make_pair(
            static_cast<int>(std::min(currentSelectionSizeT.first, countSizeT)),
            static_cast<int>(std::min(currentSelectionSizeT.second, countSizeT)));

        m_ui->startDateSlider->setRange(0, count - 1);
        m_ui->endDateSlider->setRange(0, count - 1);

        QStringList tsStrings;
        tsStrings.reserve(count);
        for (size_t i = 0; i < countSizeT; ++i)
        {
            tsStrings.push_back(QString::number(timeSteps[i]));
        }
        m_ui->startDateCombo->addItems(tsStrings);
        m_ui->endDateCombo->addItems(tsStrings);

        m_ui->startDateSlider->setSliderPosition(currentSelection.first);
        m_ui->startDateCombo->setCurrentIndex(currentSelection.first);
        m_ui->endDateSlider->setSliderPosition(currentSelection.second);
        m_ui->endDateCombo->setCurrentIndex(currentSelection.second);

        const auto comboIndexChanged = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

        guiConnect(m_ui->startDateCombo, comboIndexChanged,
            this, &RenderPropertyConfigWidget::setStartDate);
        guiConnect(m_ui->startDateSlider, &QSlider::valueChanged,
            this, &RenderPropertyConfigWidget::setStartDate);
        guiConnect(m_ui->endDateCombo, comboIndexChanged,
            this, &RenderPropertyConfigWidget::setEndDate);
        guiConnect(m_ui->endDateSlider, &QSlider::valueChanged,
            this, &RenderPropertyConfigWidget::setEndDate);
    }

    m_propertyRoot = m_content->createConfigGroup();
    m_ui->propertyBrowser->setRoot(m_propertyRoot.get());
    m_ui->propertyBrowser->resizeColumnToContents(0);

    update();
}

void RenderPropertyConfigWidget::updateTitle()
{
    const auto && title = !m_content
        ? "(No object selected)"
        : QString::number(m_renderView->index()) + ": " + m_content->dataObject().name();

    m_ui->relatedDataObject->setText(title);
}
