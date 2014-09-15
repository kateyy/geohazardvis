#include "ScalarMappingChooser.h"
#include "ui_ScalarMappingChooser.h"

#include <cassert>
#include <limits>

#include <QDir>
#include <QDebug>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>

#include <vtkLookupTable.h>

#include <core/data_objects/DataObject.h>
#include <core/scalar_mapping/ScalarsForColorMapping.h>
#include <core/scalar_mapping/ScalarToColorMapping.h>

#include <gui/widgets/ScalarRearrangeObjects.h>


ScalarMappingChooser::ScalarMappingChooser(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_ScalarMappingChooser())
    , m_mapping(nullptr)
{
    m_ui->setupUi(this);

    updateTitle();

    loadGradientImages();

    setMapping();

    connect(m_ui->rearrangeDataObjectsBtn, &QAbstractButton::clicked, this, &ScalarMappingChooser::rearrangeDataObjects);
}

ScalarMappingChooser::~ScalarMappingChooser()
{
    delete m_ui;
}

void ScalarMappingChooser::setMapping(QString rendererName, ScalarToColorMapping * mapping)
{
    updateTitle(rendererName);

    // setup gradient for newly created mappings
    if (mapping && !mapping->originalGradient())
        mapping->setGradient(selectedGradient());

    rebuildGui(mapping);

    updateGuiValueRanges();

    emit renderSetupChanged();
}

const ScalarToColorMapping * ScalarMappingChooser::mapping() const
{
    return m_mapping;
}

void ScalarMappingChooser::scalarsSelectionChanged(QString scalarsName)
{
    if (!m_mapping)
        return;

    m_mapping->setCurrentScalarsByName(scalarsName);
    updateGuiValueRanges();

    bool gradients = m_mapping->currentScalars()->dataMinValue() != m_mapping->currentScalars()->dataMaxValue();
    m_ui->gradientGroupBox->setEnabled(gradients);
    if (gradients)
        m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void ScalarMappingChooser::gradientSelectionChanged(int /*selection*/)
{
    if (!m_mapping)
        return;

    m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void ScalarMappingChooser::minValueChanged(double value)
{
    if (!m_mapping)
        return;

    m_mapping->currentScalars()->setMinValue(value);

    emit renderSetupChanged();
}

void ScalarMappingChooser::maxValueChanged(double value)
{
    if (!m_mapping)
        return;

    m_mapping->currentScalars()->setMaxValue(value);

    emit renderSetupChanged();
}

vtkLookupTable * ScalarMappingChooser::selectedGradient() const
{
    return m_gradients[m_ui->gradientComboBox->currentIndex()];
}

void ScalarMappingChooser::rearrangeDataObjects()
{
    if (!m_mapping)
        return;

    ScalarRearrangeObjects::rearrange(this, m_mapping->currentScalars());
}

void ScalarMappingChooser::loadGradientImages()
{
    // navigate to the gradient directory
    QDir dir;
    if (!dir.cd("data/gradients"))
    {
        qDebug() << "gradient directory does not exist; no gradients will be available";
        return;
    }

    // only retrieve png and jpeg files
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg";
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();

    QComboBox * gradientComboBox = m_ui->gradientComboBox;
    // load the files and add them to the combobox
    gradientComboBox->blockSignals(true);

    for (QFileInfo fileInfo : list)
    {
        QString fileName = fileInfo.baseName();
        QString filePath = fileInfo.absoluteFilePath();
        QPixmap pixmap = QPixmap(filePath).scaled(200, 20);
        m_gradients << vtkSmartPointer<vtkLookupTable>::Take(buildLookupTable(pixmap.toImage()));

        gradientComboBox->addItem(pixmap, "");
        QVariant fileVariant(filePath);
        gradientComboBox->setItemData(gradientComboBox->count() - 1, fileVariant, Qt::UserRole);
    }

    gradientComboBox->setIconSize(QSize(200, 20));
    gradientComboBox->blockSignals(false);
    // set the "default" gradient
    gradientComboBox->setCurrentIndex(32);
}

int ScalarMappingChooser::gradientIndex(vtkLookupTable * gradient) const
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

void ScalarMappingChooser::updateTitle(QString rendererName)
{
    QString title;
    if (rendererName.isEmpty())
        title = "(no render view selected)";
    else
        title = rendererName;

    m_ui->relatedRenderView->setText(title);
}

void ScalarMappingChooser::rebuildGui(ScalarToColorMapping * newMapping)
{
    m_mapping = nullptr;    // disable GUI to mapping events

    m_ui->scalarsComboBox->clear();
    m_ui->gradientGroupBox->setEnabled(false);

    // stop with cleared GUI when not rendering
    if (!newMapping)
        return;

    m_ui->scalarsComboBox->addItems(newMapping->scalarsNames());
    m_ui->scalarsComboBox->setCurrentText(newMapping->currentScalarsName());

    m_ui->gradientComboBox->setCurrentIndex(gradientIndex(newMapping->originalGradient()));
    m_ui->gradientGroupBox->setEnabled(newMapping->currentScalarsUseMappingLegend());

    // the mapping can now receive signals from the UI
    m_mapping = newMapping;
}

void ScalarMappingChooser::updateGuiValueRanges()
{
    double min = 0, max = 0;
    double currentMin = 0, currentMax = 0;

    bool enableRangeGui = false;

    if (m_mapping)
    {
        min = m_mapping->currentScalars()->dataMinValue();
        max = m_mapping->currentScalars()->dataMaxValue();
        currentMin = m_mapping->currentScalars()->minValue();
        currentMax = m_mapping->currentScalars()->maxValue();

        // assume that the mapping does not use scalar values/ranges, if it has useless min/max values
        enableRangeGui = min != max;
    }

    // disable mapping updates
    auto currentMapping = m_mapping;
    m_mapping = nullptr;

    m_ui->minValueSpinBox->setMinimum(min);
    m_ui->minValueSpinBox->setMaximum(max);
    m_ui->minValueSpinBox->setValue(currentMin);
    m_ui->maxValueSpinBox->setMinimum(min);
    m_ui->maxValueSpinBox->setMaximum(max);
    m_ui->maxValueSpinBox->setValue(currentMax);

    m_ui->minValueSpinBox->setEnabled(enableRangeGui);
    m_ui->maxValueSpinBox->setEnabled(enableRangeGui);

    m_mapping = currentMapping;
}

vtkLookupTable * ScalarMappingChooser::buildLookupTable(const QImage & image)
{   
    // use alpha = 1.0, if the image doesn't have a alpha channel
    int alphaMask = image.hasAlphaChannel() ? 0x00 : 0xFF;

    vtkLookupTable * lut = vtkLookupTable::New();
    lut->SetNumberOfTableValues(image.width());
    for (int i = 0; i < image.width(); ++i)
    {
        QRgb color = image.pixel(i, 0);
        lut->SetTableValue(i, qRed(color) / 255.0, qGreen(color) / 255.0, qBlue(color) / 255.0, (alphaMask | qAlpha(color)) / 255.0);
    }

    return lut;
}
