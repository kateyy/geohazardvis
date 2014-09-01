#include "DataChooser.h"
#include "ui_DataChooser.h"

#include <cassert>
#include <limits>

#include <QDir>
#include <QDebug>

#include <vtkLookupTable.h>

#include <core/data_mapping/ScalarsForColorMapping.h>
#include <core/data_mapping/ScalarToColorMapping.h>


DataChooser::DataChooser(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_DataChooser())
    , m_mapping(nullptr)
{
    m_ui->setupUi(this);

    updateWindowTitle();

    loadGradientImages();

    setMapping();
}

DataChooser::~DataChooser()
{
    delete m_ui;
}

void DataChooser::setMapping(QString rendererName, ScalarToColorMapping * mapping)
{
    updateWindowTitle(rendererName);

    if (m_mapping != mapping)
        rebuildGui(mapping);

    updateGuiValueRanges();

    emit renderSetupChanged();
}

const ScalarToColorMapping * DataChooser::mapping() const
{
    return m_mapping;
}

void DataChooser::scalarsSelectionChanged(QString scalarsName)
{
    if (!m_mapping)
        return;

    m_mapping->setCurrentScalarsByName(scalarsName);
    updateGuiValueRanges();

    bool gradients = m_mapping->currentScalars()->usesGradients();
    m_ui->gradientGroupBox->setEnabled(gradients);
    if (gradients)
        m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void DataChooser::gradientSelectionChanged(int /*selection*/)
{
    if (!m_mapping)
        return;

    m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void DataChooser::minValueChanged(double value)
{
    if (!m_mapping)
        return;

    m_mapping->currentScalars()->setMinValue(value);

    emit renderSetupChanged();
}

void DataChooser::maxValueChanged(double value)
{
    if (!m_mapping)
        return;

    m_mapping->currentScalars()->setMaxValue(value);

    emit renderSetupChanged();
}

vtkLookupTable * DataChooser::selectedGradient() const
{
    return m_gradients[m_ui->gradientComboBox->currentIndex()];
}

void DataChooser::loadGradientImages()
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

void DataChooser::updateWindowTitle(QString objectName)
{
    const QString defaultTitle = "scalar mapping";

    if (objectName.isEmpty())
    {
        setWindowTitle(defaultTitle);
        return;
    }

    setWindowTitle(defaultTitle + ": " + objectName);
}

void DataChooser::rebuildGui(ScalarToColorMapping * newMapping)
{
    m_mapping = nullptr;    // disable GUI to mapping events

    m_ui->scalarsComboBox->clear();
    m_ui->gradientGroupBox->setDisabled(true);

    // stop with cleared GUI when not rendering
    if (!newMapping)
        return;

    m_ui->scalarsComboBox->addItems(newMapping->scalarsNames());
    m_ui->scalarsComboBox->setCurrentText(newMapping->currentScalarsName());

    newMapping->setGradient(selectedGradient());

    m_ui->gradientGroupBox->setEnabled(newMapping->currentScalars()->usesGradients());

    // the mapping can now receive signals from the UI
    m_mapping = newMapping;
}

void DataChooser::updateGuiValueRanges()
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

vtkLookupTable * DataChooser::buildLookupTable(const QImage & image)
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
