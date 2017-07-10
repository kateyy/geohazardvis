#include "ColorMappingChooser.h"
#include "ui_ColorMappingChooser.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <iterator>

#include <QColorDialog>
#include <QDir>
#include <QDebug>
#include <QDialog>

#include <vtkCommand.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/color_mapping/GradientResourceManager.h>
#include <core/ThirdParty/alphanum.hpp>
#include <core/utility/qthelper.h>
#include <core/utility/ScalarBarActor.h>
#include <core/utility/macros.h>

#include <gui/data_view/AbstractRenderView.h>


ColorMappingChooser::ColorMappingChooser(QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_ui{ std::make_unique<Ui_ColorMappingChooser>() }
    , m_renderView{ nullptr }
    , m_mapping{ nullptr }
    , m_inAdjustLegendPosition{ false }
{
    m_ui->setupUi(this);

    loadGradientImages();

    m_ui->legendPositionComboBox->addItems({
        "left", "right", "top", "bottom", "user-defined position"
    });
    m_ui->legendPositionComboBox->setItemData(0, ColorBarRepresentation::posLeft);
    m_ui->legendPositionComboBox->setItemData(1, ColorBarRepresentation::posRight);
    m_ui->legendPositionComboBox->setItemData(2, ColorBarRepresentation::posTop);
    m_ui->legendPositionComboBox->setItemData(3, ColorBarRepresentation::posBottom);
    m_ui->legendPositionComboBox->setItemData(4, ColorBarRepresentation::posUserDefined);
    m_ui->legendPositionComboBox->setCurrentIndex(4);

    rebuildGui();
}

ColorMappingChooser::~ColorMappingChooser()
{
    setCurrentRenderView(nullptr);
}

QString ColorMappingChooser::selectedGradientName() const
{
    return m_ui->gradientComboBox->currentData().toString();
}

void ColorMappingChooser::setCurrentRenderView(AbstractRenderView * renderView)
{
    if (m_renderView == renderView)
    {
        return;
    }

    disconnectAll(m_viewConnections);
    disconnectAll(m_mappingConnections);
    discardGuiConnections();

    m_renderView = renderView;

    setSelectedVisualization(m_renderView ? m_renderView->visualzationSelection().visualization : nullptr);

    if (m_renderView)
    {
        m_viewConnections.emplace_back(connect(renderView, &AbstractRenderView::beforeDeleteVisualizations,
            this, &ColorMappingChooser::checkRemovedData));
        m_viewConnections.emplace_back(connect(m_renderView, &AbstractRenderView::visualizationSelectionChanged,
            [this] (AbstractRenderView * DEBUG_ONLY(view), const VisualizationSelection & selection) {
            assert(view == m_renderView);
            setSelectedVisualization(selection.visualization);
        }));
        m_viewConnections.emplace_back(connect(this, &ColorMappingChooser::renderSetupChanged, m_renderView, &AbstractRenderView::render));
    }
}

void ColorMappingChooser::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView || !dataObject)
    {
        setSelectedVisualization(nullptr);
        return;
    }

    const auto vis = m_renderView->visualizationFor(
        dataObject,
        m_renderView->activeSubViewIndex());

    if (!vis)
    {
        // An object was selected that is not contained in the current view,
        // so stick to the current selection.
        return;
    }

    setSelectedVisualization(vis);
}

void ColorMappingChooser::setSelectedVisualization(AbstractVisualizedData * visualization)
{
    assert(m_mapping ||
        (m_colorLegendObserverIds.empty() && m_guiConnections.empty() && !m_dataMinMaxChangedConnection));

    auto newMapping = visualization ? &visualization->colorMapping() : nullptr;

    if (newMapping == m_mapping)
    {
        return;
    }

    disconnectAll(m_mappingConnections);
    discardGuiConnections();

    for (const auto & observerIt : m_colorLegendObserverIds)
    {
        if (observerIt.first)
        {
            observerIt.first->RemoveObserver(observerIt.second);
        }
    }
    m_colorLegendObserverIds.clear();

    m_ui->legendPositionComboBox->setCurrentText("user-defined position");

    m_mapping = newMapping;

    if (m_mapping)
    {
        auto addObserver = [this] (vtkObject * subject, void(ColorMappingChooser::* callback)()) {
            m_colorLegendObserverIds.emplace(subject,
                subject->AddObserver(vtkCommand::ModifiedEvent, this, callback));
        };

        m_mappingConnections.emplace_back(connect(
            &m_mapping->colorBarRepresentation(), &ColorBarRepresentation::positionChanged,
            this, &ColorMappingChooser::colorLegendPositionChanged));

        m_mappingConnections.emplace_back(connect(
            &m_mapping->colorBarRepresentation(), &ColorBarRepresentation::colorBarVisibilityChanged,
            this, &ColorMappingChooser::updateScalarsSelection));

        addObserver(legend().GetTitleTextProperty(), &ColorMappingChooser::updateLegendTitleFont);
        addObserver(legend().GetLabelTextProperty(), &ColorMappingChooser::updateLegendLabelFont);
        addObserver(&legend(), &ColorMappingChooser::updateLegendConfig);

        m_mappingConnections.emplace_back(connect(m_mapping, &ColorMapping::scalarsChanged, [this] () {
            rebuildGui();
            emit renderSetupChanged();
        }));
        // in case the active mapping is changed via the C++ interface
        m_mappingConnections.emplace_back(connect(m_mapping, &ColorMapping::currentScalarsChanged, this, &ColorMappingChooser::mappingScalarsChanged));
    }

    rebuildGui();
    emit renderSetupChanged();
}

void ColorMappingChooser::guiScalarsSelectionChanged()
{
    // The following call is only required if scalars where changed via the GUI.
    // If current scalars / current state where changed directly via ColorMapping interface, it should return immediately.
    m_mapping->setCurrentScalarsByName(m_ui->scalarsComboBox->currentData().toString());

    updateScalarsSelection();

    emit renderSetupChanged();
}

void ColorMappingChooser::guiGradientSelectionChanged()
{
    assert(m_mapping);

    m_mapping->setGradient(selectedGradientName());

    updateNanColorButtonStyle(m_mapping->gradient()->GetNanColorAsUnsignedChars());

    emit renderSetupChanged();
}

void ColorMappingChooser::guiComponentChanged(int index)
{
    assert(m_mapping);

    m_mapping->currentScalars().setDataComponent(index);

    updateGuiValueRanges();

    emit renderSetupChanged();
}

void ColorMappingChooser::guiMinValueChanged(double value)
{
    assert(m_mapping);

    auto & scalars = m_mapping->currentScalars();
    double correctValue = std::min(scalars.maxValue(), value);
    if (value != correctValue)
    {
        QSignalBlocker signalBlocker(m_ui->minValueSpinBox);
        m_ui->minValueSpinBox->setValue(correctValue);
    }

    scalars.setMinValue(correctValue);

    emit renderSetupChanged();
}

void ColorMappingChooser::guiMaxValueChanged(double value)
{
    assert(m_mapping);

    auto & scalars = m_mapping->currentScalars();
    double correctValue = std::max(scalars.minValue(), value);
    if (value != correctValue)
    {
        QSignalBlocker signalBlocker(m_ui->maxValueSpinBox);
        m_ui->maxValueSpinBox->setValue(correctValue);
    }

    scalars.setMaxValue(correctValue);

    emit renderSetupChanged();
}

void ColorMappingChooser::guiResetMinToData()
{
    assert(m_mapping);

    m_ui->minValueSpinBox->setValue(m_mapping->currentScalars().dataMinValue());
}

void ColorMappingChooser::guiResetMaxToData()
{
    assert(m_mapping);

    m_ui->maxValueSpinBox->setValue(m_mapping->currentScalars().dataMaxValue());
}

void ColorMappingChooser::guiLegendPositionChanged()
{
    assert(m_mapping);

    if (m_inAdjustLegendPosition)
    {
        return;
    }

    m_inAdjustLegendPosition = true;

    const auto position =
        m_ui->legendPositionComboBox->currentData().value<ColorBarRepresentation::Position>();

    m_mapping->colorBarRepresentation().setPosition(position);

    emit renderSetupChanged();

    m_inAdjustLegendPosition = false;
}

void ColorMappingChooser::guiLegendTitleChanged()
{
    assert(m_mapping);

    const auto uiTitle = m_ui->legendTitleEdit->text();
    const auto newTitle = uiTitle.isEmpty() ? m_ui->legendTitleEdit->placeholderText() : uiTitle;

    m_mapping->colorBarRepresentation().actor().SetTitle(newTitle.toUtf8().data());

    emit renderSetupChanged();
}

void ColorMappingChooser::colorLegendPositionChanged()
{
    if (m_inAdjustLegendPosition)
    {
        return;
    }

    m_inAdjustLegendPosition = true;

    const auto position = m_mapping->colorBarRepresentation().position();
    const QSignalBlocker positionComboBlocker{ m_ui->legendPositionComboBox };
    const QSignalBlocker aspectRatioBlocker{ m_ui->legendAspectRatio };
    m_ui->legendPositionComboBox->setCurrentIndex(
        m_ui->legendPositionComboBox->findData(position));
    m_ui->legendAspectRatio->setValue(legend().GetAspectRatio());

    emit renderSetupChanged();

    m_inAdjustLegendPosition = false;
}

void ColorMappingChooser::updateLegendTitleFont()
{
    m_ui->legendTitleFontSize->setValue(
        legend().GetTitleTextProperty()->GetFontSize());
}

void ColorMappingChooser::updateLegendLabelFont()
{
    m_ui->legendLabelFontSize->setValue(
        legend().GetLabelTextProperty()->GetFontSize());
}

void ColorMappingChooser::updateLegendConfig()
{
    if (!m_inAdjustLegendPosition)
    {
        m_ui->legendPositionComboBox->setCurrentIndex(
            m_ui->legendPositionComboBox->findData(m_mapping->colorBarRepresentation().position()));
        m_ui->legendAspectRatio->setValue(legend().GetAspectRatio());
    }
    m_ui->legendAlignTitleCheckBox->setChecked(legend().GetTitleAlignedWithColorBar());
    m_ui->legendTransparentBackground->setChecked(legend().GetDrawBackground() != 1);
    m_ui->legendTickMarksCheckBox->setChecked(legend().GetDrawTickMarks() != 0);
    m_ui->legendTickLabelsCheckBox->setChecked(legend().GetDrawTickLabels() != 0);
    m_ui->legendSubTickMarksCheckBox->setChecked(legend().GetDrawSubTickMarks() != 0);
    m_ui->legendNumLabelsSpinBox->setValue(legend().GetNumberOfLabels());
    m_ui->legendRangeLabelsCheckBox->setChecked(legend().GetAddRangeLabels() != 0);
}

void ColorMappingChooser::updateNanColorButtonStyle(const QColor & color)
{
    m_ui->nanColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
}

void ColorMappingChooser::updateNanColorButtonStyle(const unsigned char color[4])
{
    updateNanColorButtonStyle(
        QColor(static_cast<int>(color[0]), static_cast<int>(color[1]), static_cast<int>(color[2]), static_cast<int>(color[3])));

}

void ColorMappingChooser::loadGradientImages()
{
    const auto & gradients = GradientResourceManager::instance().gradients();

    auto & gradientComboBox = *m_ui->gradientComboBox;

    // load the files and add them to the combobox
    QSignalBlocker signalBlocker(gradientComboBox);

    for (auto && gradient : gradients)
    {
        gradientComboBox.addItem(gradient.second.pixmap, "");
        gradientComboBox.setItemData(gradientComboBox.count() - 1, gradient.first);
    }

    gradientComboBox.setIconSize(gradients.begin()->second.pixmap.size());
}

void ColorMappingChooser::checkRemovedData(const QList<AbstractVisualizedData *> & content)
{
    if (!m_mapping)
    {
        return;
    }

    auto mappingVis = m_mapping->visualizedData();
    std::sort(mappingVis.begin(), mappingVis.end());
    auto con = content;
    std::sort(con.begin(), con.end());

    std::vector<AbstractVisualizedData *> intersection;
    std::set_intersection(con.begin(), con.end(),
        mappingVis.begin(), mappingVis.end(),
        std::back_inserter(intersection));

    if (!intersection.empty())
    {
        setSelectedData(nullptr);
    }
}

void ColorMappingChooser::updateTitle()
{
    QString title;
    if (!m_mapping)
    {
        title = "<b>(No object selected)</b>";
    }
    else
    {
        title = "<b>" + QString::number(m_renderView->index()) + ": ";
        for (auto && vis : m_mapping->visualizedData())
        {
            title += vis->dataObject().name() + ", ";
        }
        title.chop(2);
        title += "</b>";
    }

    m_ui->relatedRenderView->setText(title);
}

void ColorMappingChooser::guiSelectNanColor()
{
    assert(m_mapping);
    auto gradient = m_mapping->gradient();
    assert(gradient);
    auto nanColorV = gradient->GetNanColorAsUnsignedChars();
    QColor nanColor(static_cast<int>(nanColorV[0]), static_cast<int>(nanColorV[1]), static_cast<int>(nanColorV[2]), static_cast<int>(nanColorV[3]));

    // transparent NaN-color currently not correctly supported in VTK (whole object will be considered transparent)
    nanColor = QColorDialog::getColor(nanColor, nullptr, "Select a color for NaN-values"
        /*, QColorDialog::ShowAlphaChannel*/);

    if (!nanColor.isValid())
    {
        return;
    }

    double colorV[4] = { nanColor.redF(), nanColor.greenF(), nanColor.blueF(), nanColor.alphaF() };
    gradient->SetNanColor(colorV);
    gradient->BuildSpecialColors();
    updateNanColorButtonStyle(nanColor);

    emit renderSetupChanged();
}

void ColorMappingChooser::rebuildGui()
{
    discardGuiConnections();

    updateTitle();

    m_ui->scalarsGroupBox->setEnabled(false);
    m_ui->scalarsComboBox->clear();
    m_ui->gradientGroupBox->setEnabled(false);
    m_ui->nanColorButton->setStyleSheet("");
    m_ui->legendGroupBox->setEnabled(false);
    m_ui->legendGroupBox->setChecked(false);
    m_ui->legendTitleEdit->setText("");
    m_ui->legendTitleEdit->setPlaceholderText("");

    auto scalars = m_mapping ? m_mapping->scalars() : std::vector<ColorMappingData *>{};

    if (!scalars.empty())
    {
        m_ui->scalarsGroupBox->setEnabled(true);

        std::sort(scalars.begin(), scalars.end(),
            [] (const ColorMappingData * lhs, const ColorMappingData * rhs)
        {
            assert(lhs && rhs);
            return doj::alphanum_less<QString>()(lhs->name(), rhs->name());
        });
        int currentIndex = -1;
        int i = 0;
        for (const auto & sc : scalars)
        {
            auto visibleName = sc->name();
            if (sc->isTemporalAttribute())
            {
                visibleName += " (temporal)";
            }
            m_ui->scalarsComboBox->addItem(visibleName, sc->name());
            if (sc == &m_mapping->currentScalars())
            {
                currentIndex = i;
            }
            ++i;
        }
        m_ui->scalarsComboBox->setCurrentIndex(currentIndex);

        updateLegendTitleFont();
        updateLegendLabelFont();
        updateLegendConfig();

        updateNanColorButtonStyle(m_mapping->gradient()->GetNanColorAsUnsignedChars());
    }

    // this includes (and must include) setting up GUI connections
    updateScalarsSelection();
}

void ColorMappingChooser::updateScalarsSelection()
{
    discardGuiConnections();

    updateScalarsEnabled();

    if (m_mapping)
    {
        const bool usesGradients = m_mapping->currentScalarsUseMappingLegend();
        m_ui->gradientGroupBox->setEnabled(usesGradients);
        m_ui->legendGroupBox->setEnabled(usesGradients);
        m_ui->legendGroupBox->setChecked(m_mapping->colorBarRepresentation().isVisible());
        m_ui->legendTitleEdit->setPlaceholderText(m_mapping->currentScalarsName());

        QSignalBlocker signalBlocker(m_ui->gradientComboBox);
        m_ui->gradientComboBox->setCurrentIndex(
            m_ui->gradientComboBox->findData(m_mapping->gradientName()));

        if (!m_ui->legendTitleEdit->text().isEmpty())
        {   // override default title setup if user set a title
            m_mapping->colorBarRepresentation().actor().SetTitle(
                m_ui->legendTitleEdit->text().toUtf8().data());
        }
    }

    // this includes (and must include) setting up GUI connections
    updateGuiValueRanges();
}

void ColorMappingChooser::updateScalarsEnabled()
{
    const bool newState = m_mapping ? m_mapping->isEnabled() : false;

    m_ui->scalarsGroupBox->setChecked(newState);

    // When switching the state here, the internal ColorMappingData in the m_mapping changes.
    // Make sure that the UI provides valid values if we are currently enabling the mapping.
    if (newState)
    {
        updateGuiValueRanges();
    }
}

void ColorMappingChooser::setupGuiConnections()
{
    assert(m_mapping);

    m_guiConnections.emplace_back(connect(m_ui->scalarsComboBox, &QComboBox::currentTextChanged, this, &ColorMappingChooser::guiScalarsSelectionChanged));

    // all further connections are only relevant if there is currently something to configure
    // don't depend on ColorMapping to always have current scalars
    if (!m_mapping->scalarsAvailable())
    {
        return;
    }

    const auto && dSpinBoxValueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    const auto && spinBoxValueChanged = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    const auto && comboBoxIndexChanged = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

    m_guiConnections.emplace_back(connect(m_ui->scalarsGroupBox, &QGroupBox::toggled, m_mapping, &ColorMapping::setEnabled));
    m_guiConnections.emplace_back(connect(m_ui->componentComboBox, comboBoxIndexChanged, this, &ColorMappingChooser::guiComponentChanged));
    m_guiConnections.emplace_back(connect(m_ui->minValueSpinBox, dSpinBoxValueChanged, this, &ColorMappingChooser::guiMinValueChanged));
    m_guiConnections.emplace_back(connect(m_ui->maxValueSpinBox, dSpinBoxValueChanged, this, &ColorMappingChooser::guiMaxValueChanged));
    m_guiConnections.emplace_back(connect(m_ui->minLabel, &QLabel::linkActivated, this, &ColorMappingChooser::guiResetMinToData));
    m_guiConnections.emplace_back(connect(m_ui->maxLabel, &QLabel::linkActivated, this, &ColorMappingChooser::guiResetMaxToData));
    m_guiConnections.emplace_back(connect(m_ui->gradientComboBox, &QComboBox::currentTextChanged, this, &ColorMappingChooser::guiGradientSelectionChanged));
    m_guiConnections.emplace_back(connect(m_ui->nanColorButton, &QAbstractButton::pressed, this, &ColorMappingChooser::guiSelectNanColor));
    m_guiConnections.emplace_back(connect(m_ui->legendPositionComboBox, comboBoxIndexChanged, this, &ColorMappingChooser::guiLegendPositionChanged));

    m_guiConnections.emplace_back(connect(m_ui->legendTitleFontSize, spinBoxValueChanged, [this] (int fontSize) {
        auto property = legend().GetTitleTextProperty();
        auto currentSize = property->GetFontSize();
        if (currentSize == fontSize)
        {
            return;
        }
        property->SetFontSize(fontSize);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendLabelFontSize, spinBoxValueChanged, [this] (int fontSize) {
        auto property = legend().GetLabelTextProperty();
        auto currentSize = property->GetFontSize();
        if (currentSize == fontSize)
        {
            return;
        }
        property->SetFontSize(fontSize);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendAspectRatio, dSpinBoxValueChanged, [this] () {
        legend().SetAspectRatio(m_ui->legendAspectRatio->value());
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendAlignTitleCheckBox, &QAbstractButton::toggled, [this] (bool checked) {
        bool currentlyAligned = legend().GetTitleAlignedWithColorBar();
        if (currentlyAligned == checked)
        {
            return;
        }
        legend().SetTitleAlignedWithColorBar(checked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendTransparentBackground,
        &QAbstractButton::toggled, [this] (bool transparencyChecked) {
        const bool currentlyOn = legend().GetDrawBackground();
        if (currentlyOn != transparencyChecked)
        {
            return;
        }
        legend().SetDrawBackground(!transparencyChecked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendNumLabelsSpinBox, spinBoxValueChanged, [this] () {
        legend().SetNumberOfLabels(m_ui->legendNumLabelsSpinBox->value());
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendGroupBox, &QGroupBox::toggled, [this] (bool checked) {
        m_mapping->colorBarRepresentation().setVisible(checked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendTickMarksCheckBox, &QCheckBox::toggled, [this] (bool checked) {
        legend().SetDrawTickMarks(checked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendTickLabelsCheckBox, &QCheckBox::toggled, [this] (bool checked) {
        legend().SetDrawTickLabels(checked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendSubTickMarksCheckBox, &QCheckBox::toggled, [this] (bool checked) {
        legend().SetDrawSubTickMarks(checked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendRangeLabelsCheckBox, &QCheckBox::toggled, [this] (bool checked) {
        legend().SetAddRangeLabels(checked);
        emit renderSetupChanged();
    }));

    m_guiConnections.emplace_back(connect(m_ui->legendTitleEdit, &QLineEdit::editingFinished, this, &ColorMappingChooser::guiLegendTitleChanged));

    setupValueRangeConnections();
}

void ColorMappingChooser::discardGuiConnections()
{
    disconnectAll(m_guiConnections);
    discardValueRangeConnections();
}

void ColorMappingChooser::setupValueRangeConnections()
{
    auto & currentScalars = m_mapping->currentScalars();

    // depending on the effect of this change, the effect may be the same as setting different scalars
    // e.g., changes between range (n, n) and (n, m)
    m_dataMinMaxChangedConnection = connect(&currentScalars, &ColorMappingData::dataMinMaxChanged,
        this, &ColorMappingChooser::guiScalarsSelectionChanged);
}

void ColorMappingChooser::discardValueRangeConnections()
{
    disconnect(m_dataMinMaxChangedConnection);
    m_dataMinMaxChangedConnection = {};
}

void ColorMappingChooser::mappingScalarsChanged()
{
    assert(m_mapping);
    assert(m_mapping == sender());

    // for consistency, follow the same update steps as for changing the UI parameters directly
    const int uiIndex = m_ui->scalarsComboBox->findData(m_mapping->currentScalarsName());
    m_ui->scalarsComboBox->setCurrentIndex(uiIndex);

    updateScalarsEnabled();

    emit renderSetupChanged();
}

void ColorMappingChooser::updateGuiValueRanges()
{
    discardGuiConnections();

    int currentComponent = 0;
    std::vector<QString> componentNames;
    double min = 0, max = 0;
    double currentMin = 0, currentMax = 0;

    bool enableRangeGui = false;

    if (m_mapping && m_mapping->scalarsAvailable())
    {
        auto & scalars = m_mapping->currentScalars();
        for (int i = 0; i < scalars.numDataComponents(); ++i)
        {
            componentNames.push_back(scalars.componentName(i));
        }
        currentComponent = scalars.dataComponent();
        min = scalars.dataMinValue();
        max = scalars.dataMaxValue();
        currentMin = scalars.minValue();
        currentMax = scalars.maxValue();

        // assume that the mapping does not use scalar values/ranges, if it has useless min/max values
        enableRangeGui = min != max;
    }

    // around 100 steps to scroll through the full range, but step only on one digit
    double delta = max - min;
    double step = 0.0;
    int decimals = 0;
    if (std::abs(delta) > std::numeric_limits<double>::epsilon())
    {
        double stepLog = std::log10(delta / 50.0);
        int stepLogI = static_cast<int>(std::ceil(std::abs(stepLog)) * (stepLog >= 0.0 ? 1.0 : -1.0));
        step = std::pow(10, stepLogI);

        // display three decimals after the step digit
        decimals = std::max(0, -stepLogI + 3);
    }

    m_ui->componentComboBox->clear();
    for (auto && name : componentNames)
    {
        m_ui->componentComboBox->addItem(name);
    }
    m_ui->componentComboBox->setCurrentIndex(currentComponent);
    m_ui->minValueSpinBox->setMinimum(min);
    m_ui->minValueSpinBox->setMaximum(max);
    m_ui->minValueSpinBox->setSingleStep(step);
    m_ui->minValueSpinBox->setDecimals(decimals);
    m_ui->minValueSpinBox->setValue(currentMin);
    m_ui->maxValueSpinBox->setMinimum(min);
    m_ui->maxValueSpinBox->setMaximum(max);
    m_ui->maxValueSpinBox->setSingleStep(step);
    m_ui->maxValueSpinBox->setDecimals(decimals);
    m_ui->maxValueSpinBox->setValue(currentMax);

    m_ui->componentLabel->setText("component (" + QString::number(componentNames.size()) + ")");
    QString resetLink = enableRangeGui ? "resetToData" : "";
    m_ui->minLabel->setText(R"(min (data: <a href=")" + resetLink + R"(">)" + QString::number(min) + "</a>)");
    m_ui->maxLabel->setText(R"(max (data: <a href=")" + resetLink + R"(">)" + QString::number(max) + "</a>)");

    m_ui->componentComboBox->setEnabled(componentNames.size() > 1);
    m_ui->minValueSpinBox->setEnabled(enableRangeGui);
    m_ui->maxValueSpinBox->setEnabled(enableRangeGui);


    if (m_mapping)
    {
        m_ui->legendAlignTitleCheckBox->setChecked(legend().GetTitleAlignedWithColorBar());
        m_ui->legendTitleFontSize->setValue(legend().GetTitleTextProperty()->GetFontSize());
        m_ui->legendLabelFontSize->setValue(legend().GetLabelTextProperty()->GetFontSize());
        m_ui->legendTransparentBackground->setChecked(legend().GetDrawBackground() != 1);

        // setup GUI connections only if there is something to configure
        setupGuiConnections();
    }
}

OrientedScalarBarActor & ColorMappingChooser::legend()
{
    assert(m_mapping);
    return m_mapping->colorBarRepresentation().actor();
}
