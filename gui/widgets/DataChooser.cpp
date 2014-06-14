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

    if (m_mapping == mapping)
        return;

    m_mapping = nullptr;

    m_ui->scalarsComboBox->clear();
    m_ui->gradientGroupBox->setDisabled(true);

    if (!mapping)
        return;

    m_ui->scalarsComboBox->addItems(mapping->scalarsNames());
    m_ui->scalarsComboBox->setCurrentText(mapping->currentScalarsName());

    // reuse gradient selection, or use default
    if (mapping->gradient())
        m_ui->gradientComboBox->setCurrentIndex(
            m_scalarToColorGradients.indexOf(mapping->gradient()));
    else
        mapping->setGradient(selectedGradient());

    m_ui->gradientGroupBox->setEnabled(mapping->currentScalars()->usesGradients());

    // the mapping can now receive signals from the UI
    m_mapping = mapping;

    emit renderSetupChanged();
}

const ScalarToColorMapping * DataChooser::mapping() const
{
    return m_mapping;
}

void DataChooser::on_scalarsSelectionChanged(QString scalarsName)
{
    if (!m_mapping)
        return;

    m_mapping->setCurrentScalarsByName(scalarsName);

    if (scalarsName.isEmpty())
        return;

    bool gradients = m_mapping->currentScalars()->usesGradients();
    m_ui->gradientGroupBox->setEnabled(gradients);
    if (gradients)
        m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

void DataChooser::on_gradientSelectionChanged(int /*selection*/)
{
    if (!m_mapping)
        return;

    m_mapping->setGradient(selectedGradient());

    emit renderSetupChanged();
}

vtkLookupTable * DataChooser::selectedGradient() const
{
    return m_scalarToColorGradients[m_ui->gradientComboBox->currentIndex()];
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
        m_scalarToColorGradients << buildLookupTable(pixmap.toImage());

        gradientComboBox->addItem(pixmap, "");
        QVariant fileVariant(filePath);
        gradientComboBox->setItemData(gradientComboBox->count() - 1, fileVariant, Qt::UserRole);
    }

    gradientComboBox->setIconSize(QSize(200, 20));
    gradientComboBox->blockSignals(false);
    // set the "default" gradient
    gradientComboBox->setCurrentIndex(34);
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
