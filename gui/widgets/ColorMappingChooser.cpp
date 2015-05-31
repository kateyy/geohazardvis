#include "ColorMappingChooser.h"
#include "ui_ColorMappingChooser.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

#include <QColorDialog>
#include <QDir>
#include <QDebug>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>

#include <vtkLookupTable.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>

#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/ThirdParty/alphanum.hpp>

#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


namespace
{
    const int Default_gradient_index = 32;
}


ColorMappingChooser::ColorMappingChooser(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_ColorMappingChooser())
    , m_renderView(nullptr)
    , m_renderViewImpl(nullptr)
    , m_mapping(nullptr)
    , m_movingColorLegend(false)
{
    m_ui->setupUi(this);

    updateTitle();

    loadGradientImages();

    m_ui->legendPositionComboBox->addItems({
        "left", "right", "top", "bottom", "user-defined position"
    });
    m_ui->legendPositionComboBox->setCurrentText("right");

    setCurrentRenderView();

    connect(m_ui->scalarsComboBox, &QComboBox::currentTextChanged, this, &ColorMappingChooser::guiScalarsSelectionChanged);
    connect(m_ui->componentSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ColorMappingChooser::guiComponentChanged);
    connect(m_ui->minValueSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ColorMappingChooser::guiMinValueChanged);
    connect(m_ui->maxValueSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ColorMappingChooser::guiMaxValueChanged);
    connect(m_ui->minLabel, &QLabel::linkActivated, this, &ColorMappingChooser::guiResetMinToData);
    connect(m_ui->maxLabel, &QLabel::linkActivated, this, &ColorMappingChooser::guiResetMaxToData);
    connect(m_ui->gradientComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ColorMappingChooser::guiGradientSelectionChanged);
    connect(m_ui->nanColorButton, &QAbstractButton::pressed, this, &ColorMappingChooser::guiSelectNanColor);
    connect(m_ui->legendPositionComboBox, &QComboBox::currentTextChanged, this, &ColorMappingChooser::guiLegendPositionChanged);
}

ColorMappingChooser::~ColorMappingChooser()
{
    delete m_ui;
}

void ColorMappingChooser::setCurrentRenderView(AbstractRenderView * renderView)
{
    m_renderView = renderView;

    rebuildGui();

    m_ui->legendPositionComboBox->setCurrentText("user-defined position");

    if (m_mapping)
    {
        vtkEventQtSlotConnect * vtkQtConnect = vtkEventQtSlotConnect::New();
        m_colorLegendConnects.TakeReference(vtkQtConnect);
        vtkQtConnect->Connect(m_mapping->colorMappingLegend()->GetPositionCoordinate(), vtkCommand::ModifiedEvent,
            this, SLOT(colorLegendPositionChanged()));
        vtkQtConnect->Connect(m_mapping->colorMappingLegend()->GetPosition2Coordinate(), vtkCommand::ModifiedEvent,
            this, SLOT(colorLegendPositionChanged()));
    }
    else
    {
        m_colorLegendConnects = nullptr;
    }
}

void ColorMappingChooser::guiScalarsSelectionChanged(const QString & scalarsName)
{
    if (!m_mapping)
        return;

    m_mapping->setCurrentScalarsByName(scalarsName);
    updateGuiValueRanges();

    bool gradients = m_mapping->currentScalarsUseMappingLegend();
    m_ui->gradientGroupBox->setEnabled(gradients);
    m_ui->colorLegendCheckBox->setChecked(m_mapping->colorMappingLegendVisible());
    if (gradients)
        m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void ColorMappingChooser::guiGradientSelectionChanged(int /*selection*/)
{
    if (!m_mapping)
        return;

    m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void ColorMappingChooser::guiComponentChanged(int guiComponent)
{
    if (!m_mapping)
        return;

    auto component = guiComponent - 1;

    m_mapping->scalarsSetDataComponent(component);

    updateGuiValueRanges();

    emit renderSetupChanged();
}

void ColorMappingChooser::guiMinValueChanged(double value)
{
    if (!m_mapping)
        return;

    auto scalars = m_mapping->currentScalars();
    double correctValue = std::min(scalars->maxValue(), value);
    if (value != correctValue)
    {
        m_ui->minValueSpinBox->blockSignals(true);
        m_ui->minValueSpinBox->setValue(correctValue);
        m_ui->minValueSpinBox->blockSignals(false);
    }

    scalars->setMinValue(correctValue);

    emit renderSetupChanged();
}

void ColorMappingChooser::guiMaxValueChanged(double value)
{
    if (!m_mapping)
        return;

    auto scalars = m_mapping->currentScalars();
    double correctValue = std::max(scalars->minValue(), value);
    if (value != correctValue)
    {
        m_ui->maxValueSpinBox->blockSignals(true);
        m_ui->maxValueSpinBox->setValue(correctValue);
        m_ui->maxValueSpinBox->blockSignals(false);
    }

    scalars->setMaxValue(correctValue);

    emit renderSetupChanged();
}

void ColorMappingChooser::guiResetMinToData()
{
    if (!m_mapping)
        return;

    m_ui->minValueSpinBox->setValue(m_mapping->currentScalars()->dataMinValue());
}

void ColorMappingChooser::guiResetMaxToData()
{
    if (!m_mapping)
        return;

    m_ui->maxValueSpinBox->setValue(m_mapping->currentScalars()->dataMaxValue());
}

vtkLookupTable * ColorMappingChooser::selectedGradient() const
{
    return m_gradients.value(m_ui->gradientComboBox->currentIndex());
}

void ColorMappingChooser::guiLegendPositionChanged(const QString & position)
{
    if (!m_mapping || !m_renderView)
        return;

    vtkScalarBarRepresentation * representation = m_renderViewImpl->colorLegendWidget()->GetScalarBarRepresentation();

    m_movingColorLegend = true;

    if (position == "left")
    {
        representation->SetOrientation(1);
        representation->SetPosition(0.01, 0.1);
        representation->SetPosition2(0.17, 0.8);
    }
    else if (position == "right")
    {
        representation->SetOrientation(1);
        representation->SetPosition(0.82, 0.1);
        representation->SetPosition2(0.17, 0.8);
    }
    else if (position == "top")
    {
        representation->SetOrientation(0);
        representation->SetPosition(0.01, 0.82);
        representation->SetPosition2(0.98, 0.17);
    }
    else if (position == "bottom")
    {
        representation->SetOrientation(0);
        representation->SetPosition(0.01, 0.01);
        representation->SetPosition2(0.98, 0.17);
    }

    m_renderView->render();

    m_movingColorLegend = false;
}

void ColorMappingChooser::colorLegendPositionChanged()
{
    if (!m_movingColorLegend)
        m_ui->legendPositionComboBox->setCurrentText("user-defined position");
}

void ColorMappingChooser::loadGradientImages()
{
    const QSize gradientImageSize{ 200, 20 };

    QComboBox * gradientComboBox = m_ui->gradientComboBox;

    // load the files and add them to the combobox
    gradientComboBox->blockSignals(true);

    // navigate to the gradient directory
    QDir dir;
    if (!dir.cd("data/gradients"))
        qDebug() << "gradient directory does not exist; only a fallback gradient will be available";
    else
    {
        // only retrieve png and jpeg files
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        QStringList filters;
        filters << "*.png" << "*.jpg" << "*.jpeg";
        dir.setNameFilters(filters);
        QFileInfoList list = dir.entryInfoList();

        for (QFileInfo fileInfo : list)
        {
            QString fileName = fileInfo.baseName();
            QString filePath = fileInfo.absoluteFilePath();
            QPixmap pixmap = QPixmap(filePath).scaled(gradientImageSize);
            m_gradients << buildLookupTable(pixmap.toImage());

            gradientComboBox->addItem(pixmap, "");
        }
    }

    // fallback, in case we didn't find any gradient images
    if (m_gradients.isEmpty())
    {
        auto gradient = vtkSmartPointer<vtkLookupTable>::New();
        gradient->SetNumberOfTableValues(gradientImageSize.width());
        gradient->Build();

        QImage image(gradientImageSize, QImage::Format_RGBA8888);
        for (int i = 0; i < gradientImageSize.width(); ++i)
        {
            double colorF[4];
            gradient->GetTableValue(i, colorF);
            auto colorUI = QColor(colorF[0] * 0xFF, colorF[1] * 0xFF, colorF[2] * 0xFF, colorF[3] * 0xFF).rgba();
            for (int l = 0; l < gradientImageSize.height(); ++l)
                image.setPixel(i, l, colorUI);
        }

        m_gradients << gradient;
        gradientComboBox->addItem(QPixmap::fromImage(image), "");
    }

    gradientComboBox->setIconSize(gradientImageSize);
    gradientComboBox->blockSignals(false);
    // set the "default" gradient
    gradientComboBox->setCurrentIndex(std::min(Default_gradient_index, gradientComboBox->count() - 1));
}

int ColorMappingChooser::gradientIndex(vtkLookupTable * gradient) const
{
    int index = 0;
    for (const vtkSmartPointer<vtkLookupTable> & ptr : m_gradients)
    {
        if (ptr.Get() == gradient)
            return index;
        ++index;
    }
    assert(false);
    return -1;
}

void ColorMappingChooser::checkRenderViewColorMapping()
{
    m_renderViewImpl = nullptr;
    m_mapping = nullptr;

    if (!m_renderView)
        return;

    // color mapping is currently only implemented for 3D/render views (not for context/2D/plot views)
    m_renderViewImpl = dynamic_cast<RendererImplementationBase3D *>(&m_renderView->implementation());
    if (!m_renderViewImpl)
        return;

    m_mapping = m_renderViewImpl->colorMapping();

    // setup gradient for newly created mappings
    if (m_mapping && !m_mapping->originalGradient())
    {
        m_ui->gradientComboBox->setCurrentIndex(    // use default gradient for new mappings
            std::min(m_ui->gradientComboBox->count() - 1, Default_gradient_index));
        m_mapping->setGradient(selectedGradient());
    }
}

void ColorMappingChooser::updateTitle(QString rendererName)
{
    QString title;
    if (rendererName.isEmpty())
        title = "(No Render View selected)";
    else
        title = rendererName;

    m_ui->relatedRenderView->setText(title);
}

void ColorMappingChooser::guiSelectNanColor()
{
    assert(m_mapping);
    auto gradient = m_mapping->gradient();
    assert(gradient);
    auto nanColorV = gradient->GetNanColorAsUnsignedChars();
    QColor nanColor(static_cast<int>(nanColorV[0]), static_cast<int>(nanColorV[1]), static_cast<int>(nanColorV[2]), static_cast<int>(nanColorV[3]));

    nanColor = QColorDialog::getColor(nanColor, nullptr, "Select a color for NaN-values", QColorDialog::ShowAlphaChannel);

    if (!nanColor.isValid())
        return;

    double colorV[4] = { nanColor.redF(), nanColor.greenF(), nanColor.blueF(), nanColor.alphaF() };
    gradient->SetNanColor(colorV);
    m_ui->nanColorButton->setStyleSheet(QString("background-color: %1").arg(nanColor.name()));

    emit renderSetupChanged();
}

void ColorMappingChooser::rebuildGui()
{
    checkRenderViewColorMapping();

    updateTitle(m_renderView ? m_renderView->friendlyName() : "");

    auto newMapping = m_mapping;
    m_mapping = nullptr;    // disable GUI to mapping events

    for (auto & connection : m_qtConnect)
        disconnect(connection);
    m_qtConnect.clear();

    m_ui->scalarsComboBox->clear();
    m_ui->gradientGroupBox->setEnabled(false);
    m_ui->nanColorButton->setStyleSheet("");
    m_ui->colorLegendCheckBox->setChecked(false);

    if (m_renderView)
        m_qtConnect << connect(m_renderView, &AbstractRenderView::visualizationsChanged, this, &ColorMappingChooser::rebuildGui);

    // clear GUI when not rendering
    if (newMapping)
    {
        auto items = newMapping->scalarsNames();
        std::sort(items.begin(), items.end(), doj::alphanum_less<QString>());
        m_ui->scalarsComboBox->addItems(items);

        m_ui->scalarsComboBox->setCurrentText(newMapping->currentScalarsName());
        m_ui->gradientComboBox->setCurrentIndex(gradientIndex(newMapping->originalGradient()));
        m_ui->gradientGroupBox->setEnabled(newMapping->currentScalarsUseMappingLegend());
        m_ui->colorLegendCheckBox->setChecked(newMapping->colorMappingLegendVisible());

        const unsigned char * nanColorV = newMapping->gradient()->GetNanColorAsUnsignedChars();

        QColor nanColor(static_cast<int>(nanColorV[0]), static_cast<int>(nanColorV[1]), static_cast<int>(nanColorV[2]), static_cast<int>(nanColorV[3]));
        m_ui->nanColorButton->setStyleSheet(QString("background-color: %1").arg(nanColor.name()));

        m_qtConnect << connect(newMapping, &ColorMapping::scalarsChanged, this, &ColorMappingChooser::rebuildGui);
        m_qtConnect << connect(m_ui->colorLegendCheckBox, &QAbstractButton::toggled,
            [this, newMapping] (bool checked) {
            newMapping->setColorMappingLegendVisible(checked);
            m_renderView->render();
        });
        m_qtConnect << connect(this, &ColorMappingChooser::renderSetupChanged, 
            m_renderView, &AbstractRenderView::render);
    }

    // the mapping can now receive signals from the UI
    m_mapping = newMapping;

    updateGuiValueRanges();

    emit renderSetupChanged();
}

void ColorMappingChooser::updateGuiValueRanges()
{
    int numComponents = 0, currentComponent = 1;
    double min = 0, max = 0;
    double currentMin = 0, currentMax = 0;

    bool enableRangeGui = false;

    if (m_mapping)
    {
        ColorMappingData * scalars = m_mapping->currentScalars();

        if (scalars)
        {
            numComponents = scalars->numDataComponents();
            currentComponent = scalars->dataComponent();
            min = scalars->dataMinValue();
            max = scalars->dataMaxValue();
            currentMin = scalars->minValue();
            currentMax = scalars->maxValue();
        }

        // assume that the mapping does not use scalar values/ranges, if it has useless min/max values
        enableRangeGui = min != max;
    }

    // disable mapping updates
    auto currentMapping = m_mapping;
    m_mapping = nullptr;

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

    m_ui->componentSpinBox->setMinimum(1);
    m_ui->componentSpinBox->setMaximum(numComponents);
    m_ui->componentSpinBox->setValue(currentComponent + 1);
    m_ui->minValueSpinBox->setMinimum(min);
    m_ui->minValueSpinBox->setMaximum(max);
    m_ui->minValueSpinBox->setValue(currentMin);
    m_ui->minValueSpinBox->setSingleStep(step);
    m_ui->minValueSpinBox->setDecimals(decimals);
    m_ui->maxValueSpinBox->setMinimum(min);
    m_ui->maxValueSpinBox->setMaximum(max);
    m_ui->maxValueSpinBox->setValue(currentMax);
    m_ui->maxValueSpinBox->setSingleStep(step);
    m_ui->maxValueSpinBox->setDecimals(decimals);

    m_ui->componentLabel->setText("component (" + QString::number(numComponents) + ")");
    QString resetLink = enableRangeGui ? "resetToData" : "";
    m_ui->minLabel->setText("min (data: <a href=\"" + resetLink + "\">" + QString::number(min) + "</a>)");
    m_ui->maxLabel->setText("max (data: <a href=\"" + resetLink + "\">" + QString::number(max) + "</a>)");

    m_ui->componentSpinBox->setEnabled(numComponents > 1);
    m_ui->minValueSpinBox->setEnabled(enableRangeGui);
    m_ui->maxValueSpinBox->setEnabled(enableRangeGui);

    m_mapping = currentMapping;
}

vtkSmartPointer<vtkLookupTable> ColorMappingChooser::buildLookupTable(const QImage & image)
{
    // use alpha = 1.0, if the image doesn't have a alpha channel
    int alphaMask = image.hasAlphaChannel() ? 0x00 : 0xFF;

    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(image.width());
    for (int i = 0; i < image.width(); ++i)
    {
        QRgb color = image.pixel(i, 0);
        lut->SetTableValue(i, qRed(color) / 255.0, qGreen(color) / 255.0, qBlue(color) / 255.0, (alphaMask | qAlpha(color)) / 255.0);
    }

    lut->SetNanColor(1, 1, 1, 0);   // transparent!

    return lut;
}
