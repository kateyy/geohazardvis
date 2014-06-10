#include "DataChooser.h"
#include "ui_DataChooser.h"

#include <cassert>
#include <limits>

#include <QDir>
#include <QDebug>

#include "core/data_objects/RenderedPolyData.h"
#include "core/data_objects/DataObject.h"
#include "core/Input.h"


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

    m_ui->scalarsComboBox->clear();
    m_ui->gradientGroupBox->setDisabled(true);

    if (!mapping)
        return;

    m_mapping = mapping;

    m_ui->scalarsComboBox->addItems(m_mapping->scalarsNames());
    m_ui->scalarsComboBox->setCurrentText(m_mapping->currentScalars());

    if (m_mapping->gradient())
        m_ui->gradientComboBox->setCurrentIndex(
            m_scalarToColorGradients.indexOf(*m_mapping->gradient()));
}

const ScalarToColorMapping * DataChooser::mapping() const
{
    return m_mapping;
}

void DataChooser::on_scalarsSelectionChanged(QString scalarsName)
{
    if (!m_mapping)
        return;

    m_mapping->setCurrentScalars(scalarsName);
}

void DataChooser::on_gradientSelectionChanged(int selection)
{
    if (!m_mapping)
        return;

    int gradientIndex = m_ui->gradientComboBox->currentIndex();

    m_mapping->setGradient(&selectedGradient());
}

const QImage & DataChooser::selectedGradient() const
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
        m_scalarToColorGradients << pixmap.toImage();

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
