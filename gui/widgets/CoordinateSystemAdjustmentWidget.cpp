#include "CoordinateSystemAdjustmentWidget.h"
#include "ui_CoordinateSystemAdjustmentWidget.h"

#include <QMenu>
#include <QMessageBox>

#include <core/CoordinateSystems.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


CoordinateSystemAdjustmentWidget::CoordinateSystemAdjustmentWidget(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui{ std::make_unique<Ui_CoordinateSystemAdjustmentWidget>() }
    , m_autoSetReferencePointMenu{ std::make_unique<QMenu>() }
{
    m_ui->setupUi(this);
    setFixedSize(size());

    m_ui->coordinateSystemTypeCombo->clear();

    for (auto it : CoordinateSystemType::typeToStringMap())
    {
        m_ui->coordinateSystemTypeCombo->addItem(it.second);
    }

    connect(m_ui->referencePointGlobalCheckBox, &QCheckBox::toggled, m_ui->refPointGlobalWidget, &QWidget::setEnabled);
    connect(m_ui->referencePointLocalCheckBox, &QCheckBox::toggled, m_ui->refPointLocalWidget, &QWidget::setEnabled);

    m_actionUnsupportedType = m_autoSetReferencePointMenu->addAction("Set coordinate system type first...");
    m_actionUnsupportedType->setEnabled(false);
    m_actionRefToNorthWest = m_autoSetReferencePointMenu->addAction("North West");
    connect(m_actionRefToNorthWest, &QAction::triggered, [this] ()
    {
        if (!m_dataObject)
        {
            return;
        }

        auto spec = specFromUi();
        spec.referencePointLocalRelative = { 0.0, 0.0 };
        if (spec.type == CoordinateSystemType::geographic)
        {
            const auto targetLongLat = m_dataObject->bounds().convertTo<2>().min();
            spec.referencePointLatLong = { targetLongLat[1], targetLongLat[0] };
        }
        specToUi(spec);
    });
    m_actionRefCenter = m_autoSetReferencePointMenu->addAction("Center");
    connect(m_actionRefCenter, &QAction::triggered, [this] ()
    {
        if (!m_dataObject)
        {
            return;
        }

        auto spec = specFromUi();
        spec.referencePointLocalRelative = { 0.5, 0.5 };
        if (spec.type == CoordinateSystemType::geographic)
        {
            const auto targetLongLat = m_dataObject->bounds().convertTo<2>().center();
            spec.referencePointLatLong = { targetLongLat[1], targetLongLat[0] };
        }
        specToUi(spec);
    });
    m_actionRefOrigin = m_autoSetReferencePointMenu->addAction("Origin");
    connect(m_actionRefOrigin, &QAction::triggered, [this] ()
    {
        if (!m_dataObject)
        {
            return;
        }

        auto spec = specFromUi();
        if (spec.type != CoordinateSystemType::metricLocal)
        {
            return;
        }
        const auto bounds = m_dataObject->bounds().convertTo<2>();
        spec.referencePointLocalRelative = bounds.relativeOriginPosition();
        specToUi(spec);
    });
    connect(m_ui->autoSetReferencePointButton, &QPushButton::clicked, [this] ()
    {
        m_autoSetReferencePointMenu->exec(
            m_ui->autoSetReferencePointButton->mapToGlobal({ 0, m_ui->autoSetReferencePointButton->height() }));
    });

    m_ui->refPointGlobalWidget->setEnabled(m_ui->referencePointGlobalCheckBox->isChecked());
    m_ui->refPointLocalWidget->setEnabled(m_ui->referencePointLocalCheckBox->isChecked());

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &CoordinateSystemAdjustmentWidget::finish);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &CoordinateSystemAdjustmentWidget::reject);
}

CoordinateSystemAdjustmentWidget::~CoordinateSystemAdjustmentWidget() = default;

void CoordinateSystemAdjustmentWidget::setDataObject(CoordinateTransformableDataObject * dataObject)
{
    if (dataObject == m_dataObject)
    {
        return;
    }

    if (!dataObject)
    {
        return;
    }

    m_dataObject = dataObject;
    m_ui->dataSetNameEdit->setText(dataObject->name());
    m_ui->dataSetTypeEdit->setText(dataObject->dataTypeName());

    updateInfoText();

    connect(m_ui->coordinateSystemTypeCombo, &QComboBox::currentTextChanged, this, &CoordinateSystemAdjustmentWidget::updateInfoText);
    
    specToUi(dataObject->coordinateSystem());
}

void CoordinateSystemAdjustmentWidget::updateInfoText()
{
    if (!m_dataObject)
    {
        m_ui->coordinateValueRangesEdit->clear();
        return;
    }

    const auto currentType = CoordinateSystemType(m_ui->coordinateSystemTypeCombo->currentText());
    const auto bounds2D = m_dataObject->bounds().convertTo<2>();
    char coordinateLabel = 'X';
    QString coordsRangesText;
    for (unsigned i = 0; i < 2; ++i)
    {
        const auto range = bounds2D.extractDimension(i);
        coordsRangesText += QChar(coordinateLabel + static_cast<char>(i));
        coordsRangesText += ": " + QString::number(range.min()) + "; " + QString::number(range.max()) + "\n";
    }
    const auto center = bounds2D.center();
    coordsRangesText += "Center: " + QString::number(center[0]) + "; " + QString::number(center[1]) + "\n";

    if (currentType == CoordinateSystemType::metricLocal)
    {
        const auto relOrigin = bounds2D.relativeOriginPosition();
        coordsRangesText += "Relative Position of (0; 0): " + QString::number(relOrigin[0]) + "; " + QString::number(relOrigin[1]);
    }

    m_ui->coordinateValueRangesEdit->setPlainText(coordsRangesText);

    m_actionUnsupportedType->setVisible(currentType == CoordinateSystemType::unspecified);
    m_actionRefToNorthWest->setVisible(currentType != CoordinateSystemType::unspecified);
    m_actionRefCenter->setVisible(currentType != CoordinateSystemType::unspecified);
    m_actionRefOrigin->setVisible(currentType == CoordinateSystemType::metricLocal);
}

void CoordinateSystemAdjustmentWidget::finish()
{
    if (!m_dataObject)
    {
        reject();
    }

    const auto spec = specFromUi();

    if (!spec.isValid(false))
    {
        QMessageBox::warning(this, windowTitle(),
            "Coordinate System Specification is not valid.");
        return;
    }

    m_dataObject->specifyCoordinateSystem(spec);

    accept();
}

ReferencedCoordinateSystemSpecification CoordinateSystemAdjustmentWidget::specFromUi() const
{
    ReferencedCoordinateSystemSpecification spec;

    spec.type.fromString(m_ui->coordinateSystemTypeCombo->currentText());

    spec.geographicSystem = m_ui->geoCoordSystemEdit->text();
    spec.globalMetricSystem = m_ui->metricCoordSystemEdit->text();

    if (m_ui->referencePointGlobalCheckBox->isChecked())
    {
        spec.referencePointLatLong.SetX(m_ui->refGlobalLatitudeSpinBox->value());
        spec.referencePointLatLong.SetY(m_ui->refGlobalLongitudeSpinBox->value());
    }
    else
    {
        uninitializeVector(spec.referencePointLatLong);
    }

    if (m_ui->referencePointGlobalCheckBox->isChecked())
    {
        spec.referencePointLocalRelative.SetX(m_ui->refLocalRelativeEastingSpinBox->value());
        spec.referencePointLocalRelative.SetY(m_ui->refLocalRelativeNorthingSpinBox->value());
    }
    else
    {
        uninitializeVector(spec.referencePointLocalRelative);
    }

    return spec;
}

void CoordinateSystemAdjustmentWidget::specToUi(const ReferencedCoordinateSystemSpecification & spec)
{
    m_ui->coordinateSystemTypeCombo->setCurrentText(spec.type.toString());

    //spec.geographicSystem // not implemented yet
    //spec.globalMetricSystem // not implemented yet

    const bool hasRefPointGlobal = isVectorInitialized(spec.referencePointLatLong);
    m_ui->referencePointGlobalCheckBox->setChecked(hasRefPointGlobal);
    if (hasRefPointGlobal)
    {
        m_ui->refGlobalLatitudeSpinBox->setValue(spec.referencePointLatLong.GetX());
        m_ui->refGlobalLongitudeSpinBox->setValue(spec.referencePointLatLong.GetY());
    }
    else
    {
        m_ui->refGlobalLatitudeSpinBox->setValue(0.0);
        m_ui->refGlobalLongitudeSpinBox->setValue(0.0);
    }

    const bool hasRefPointLocal = isVectorInitialized(spec.referencePointLocalRelative);
    m_ui->referencePointLocalCheckBox->setChecked(hasRefPointLocal);
    if (hasRefPointLocal)
    {
        m_ui->refLocalRelativeEastingSpinBox->setValue(spec.referencePointLocalRelative.GetX());
        m_ui->refLocalRelativeNorthingSpinBox->setValue(spec.referencePointLocalRelative.GetY());
    }
    else
    {
        m_ui->refLocalRelativeEastingSpinBox->setValue(0.5);
        m_ui->refLocalRelativeNorthingSpinBox->setValue(0.5);
    }
}
