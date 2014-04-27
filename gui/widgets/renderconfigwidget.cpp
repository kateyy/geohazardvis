#include "renderconfigwidget.h"
#include "ui_renderconfigwidget.h"

#include <QDir>

#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

using namespace reflectionzeug;
using namespace propertyguizeug;

RenderConfigWidget::RenderConfigWidget(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_RenderConfigWidget())
, m_propertyBrowser(new PropertyBrowser())
, m_propertyRoot(nullptr)
, m_renderProperty(nullptr)
{
    m_ui->setupUi(this);
    m_ui->mainLayout->addWidget(m_propertyBrowser);

    loadGradientImages();
}

RenderConfigWidget::~RenderConfigWidget()
{
    clear();
    delete m_propertyBrowser;
    delete m_ui;
}

void RenderConfigWidget::loadGradientImages()
{
    // navigate to the gradient directory
    QDir dir;
    if (!dir.cd("data/gradients"))
    {
        std::cout << "gradient directory does not exist; no gradients will be available" << std::endl;
        return;
    }

    // only retrieve png and jpeg files
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg";
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();

    QComboBox* gradientComboBox = m_ui->gradientComboBox;
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

const QImage & RenderConfigWidget::selectedGradient() const
{
    return m_scalarToColorGradients[m_ui->gradientComboBox->currentIndex()];
}

void RenderConfigWidget::updateGradientSelection(int selection)
{
    emit gradientSelectionChanged(m_scalarToColorGradients[selection]);
}

void RenderConfigWidget::clear()
{
    setRenderProperty(nullptr);
}

void RenderConfigWidget::setRenderProperty(vtkProperty * property)
{
    m_renderProperty = property;

    updatePropertyBrowser();
}

void RenderConfigWidget::updatePropertyBrowser()
{
    m_propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;

    if (m_renderProperty == nullptr)
        return;

    vtkProperty * renderProperty = m_renderProperty;

    auto * renderSettings = new PropertyGroup("renderSettings");

    auto * faceColor = renderSettings->addProperty<Color>("color",
        [renderProperty]() {
        double * color = renderProperty->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderProperty](const Color & color) {
        renderProperty->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
    });

    m_propertyRoot = renderSettings;
    m_propertyBrowser->setRoot(renderSettings);
}
