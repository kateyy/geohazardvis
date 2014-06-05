#include "DataChooser.h"
#include "ui_DataChooser.h"

#include <QDir>
#include <QDebug>

#include "core/RenderedPolyData.h"
#include "core/DataObject.h"
#include "core/Input.h"


DataChooser::DataChooser(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_DataChooser())
{
    m_ui->setupUi(this);

    updateWindowTitle();

    loadGradientImages();
}

DataChooser::~DataChooser()
{
    delete m_ui;
}

void DataChooser::setRenderedData(std::shared_ptr<RenderedData> renderedData)
{
    updateWindowTitle(QString::fromStdString(renderedData->dataObject()->input()->name));

    m_renderedData = renderedData;

    std::shared_ptr<RenderedPolyData> renderedPolyData = std::dynamic_pointer_cast<RenderedPolyData>(m_renderedData);
    if (!renderedPolyData)
        return;

    setUiDataSelection(renderedPolyData->currentDataSelection());

    if (renderedPolyData->currentGradient())
        m_ui->gradientComboBox->setCurrentIndex(m_scalarToColorGradients.indexOf(*renderedPolyData->currentGradient()));
}

void DataChooser::updateSelection()
{
    std::shared_ptr<RenderedPolyData> renderedPolyData = std::dynamic_pointer_cast<RenderedPolyData>(m_renderedData);
    if (renderedPolyData)
    {
        renderedPolyData->setSurfaceColorMapping(
            dataSelection(),
            &m_scalarToColorGradients[m_ui->gradientComboBox->currentIndex()]);
    }

    emit selectionChanged(dataSelection());
}

void DataChooser::updateGradientSelection(int selection)
{
    std::shared_ptr<RenderedPolyData> renderedPolyData = std::dynamic_pointer_cast<RenderedPolyData>(m_renderedData);
    if (renderedPolyData)
    {
        renderedPolyData->setSurfaceColorMapping(
            dataSelection(),
            &m_scalarToColorGradients[m_ui->gradientComboBox->currentIndex()]);
    }

    emit gradientSelectionChanged(m_scalarToColorGradients[selection]);
}

DataSelection DataChooser::dataSelection() const
{
    if (m_ui->scalars_singleColor->isChecked())
        return DataSelection::DefaultColor;
    if (m_ui->scalars_xValues->isChecked())
        return DataSelection::Vertex_xValues;
    if (m_ui->scalars_yValues->isChecked())
        return DataSelection::Vertex_yValues;
    if (m_ui->scalars_zValues->isChecked())
        return DataSelection::Vertex_zValues;

    return DataSelection::NoSelection;
}

void DataChooser::setUiDataSelection(DataSelection dataSelection)
{
    switch (dataSelection)
    {
    case DataSelection::DefaultColor:
        m_ui->scalars_singleColor->setChecked(true);
        break;
    case DataSelection::Vertex_xValues:
        m_ui->scalars_xValues->setChecked(true);
        break;
    case DataSelection::Vertex_yValues:
        m_ui->scalars_yValues->setChecked(true);
        break;
    case DataSelection::Vertex_zValues:
        m_ui->scalars_zValues->setChecked(true);
        break;
    case DataSelection::NoSelection:
        m_ui->scalars_singleColor->setChecked(false);
        m_ui->scalars_xValues->setChecked(false);
        m_ui->scalars_yValues->setChecked(false);
        m_ui->scalars_zValues->setChecked(false);
        break;
    }
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

void DataChooser::updateWindowTitle(QString propertyName)
{
    const QString defaultTitle = "scalar mapping";

    if (propertyName.isEmpty())
    {
        setWindowTitle(defaultTitle);
        return;
    }

    setWindowTitle(defaultTitle + ": " + propertyName);
}
