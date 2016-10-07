#include "CoordinateSystemAdjustmentWidget.h"
#include "ui_CoordinateSystemAdjustmentWidget.h"

#include <cassert>
#include <map>

#include <QMenu>
#include <QMessageBox>

#include <core/CoordinateSystems.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{

enum RefPointQuickly
{
    refUnsupported,
    refToCenter,
    refToNorthEast,
    refToNorthWest,
    refToSouthEast,
    refToSouthWest,
    refToOrigin,
    refAdjustLocalToGeo,
    refAdjustGeoToLocal,
    RefPointQuicklyNumValues
};

const QString & refPointQuicklyName(RefPointQuickly actionType)
{
    static const std::map<RefPointQuickly, QString> map = {
        { refUnsupported, "Set coordinate system type first..." },
        { refToCenter, "Center" },
        { refToNorthEast, "North East" },
        { refToNorthWest, "North West" },
        { refToSouthEast, "South East" },
        { refToSouthWest, "South West" },
        { refToOrigin, "Origin" },
        { refAdjustLocalToGeo, "Adjust Local Reference to Geographic" },
        { refAdjustGeoToLocal, "Adjust Geographic Reference to Local" },
    };

    auto && it = map.find(actionType);
    assert(it != map.end());
    return it->second;
}

vtkVector2d refPointQuicklyToVec(RefPointQuickly actionType)
{
    switch (actionType)
    {
        // relative x,y coordinate -> "Easting", "Northing"
    case refToCenter: return{ 0.5, 0.5 };
    case refToNorthEast: return{ 1.0, 1.0 };
    case refToNorthWest: return{ 0.0, 1.0 };
    case refToSouthEast: return{ 1.0, 0.0 };
    case refToSouthWest: return{ 0.0, 0.0 };
    default:
        assert(false);
        return{ 0, 0 };
    }
}

bool refPointQuicklySupports(RefPointQuickly actionType, CoordinateSystemType coordinateSystemType)
{
    switch (actionType)
    {
    case refUnsupported:
        return coordinateSystemType == CoordinateSystemType::unspecified;
    case refToCenter:
    case refToNorthEast:
    case refToNorthWest:
    case refToSouthEast:
    case refToSouthWest:
        return coordinateSystemType != CoordinateSystemType::unspecified;
    case refToOrigin:
        return coordinateSystemType == CoordinateSystemType::metricLocal;
    case refAdjustLocalToGeo:
    case refAdjustGeoToLocal:
        return coordinateSystemType == CoordinateSystemType::geographic;
    default:
        assert(false);
        return false;
    }
}

}


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

    setupQuickSetActions();

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

    m_ui->coordinateValueRangesEdit->setPlainText(coordsRangesText);

    for (int i = 0; i < RefPointQuicklyNumValues; ++i)
    {
        const auto actionType = static_cast<RefPointQuickly>(i);
        m_refPointQuickSetActions[i]->setVisible(refPointQuicklySupports(actionType, currentType));
    }
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

void CoordinateSystemAdjustmentWidget::setupQuickSetActions()
{
    auto addRefAction = [this] (RefPointQuickly actionType) -> QAction *
    {
        auto action = m_autoSetReferencePointMenu->addAction(refPointQuicklyName(actionType));
        const auto idx = static_cast<int>(actionType);
        m_refPointQuickSetActions.resize(idx + 1);
        assert(m_refPointQuickSetActions[idx] == nullptr);
        m_refPointQuickSetActions[idx] = action;
        return action;
    };

    auto unsupportedAction = addRefAction(refUnsupported);
    unsupportedAction->setEnabled(false);

    for (auto actionType : { refToCenter, refToNorthEast, refToNorthWest, refToSouthEast, refToSouthWest })
    {
        auto action = addRefAction(actionType);
        connect(action, &QAction::triggered, [this, actionType] ()
        {
            if (!m_dataObject)
            {
                return;
            }

            auto spec = specFromUi();
            spec.referencePointLocalRelative = refPointQuicklyToVec(actionType);

            if (spec.type == CoordinateSystemType::geographic)
            {
                auto && min = m_dataObject->bounds().convertTo<2>().min();  // x: longitudes, y: latitudes
                auto && size = m_dataObject->bounds().convertTo<2>().componentSize();

                const auto refLongLat = min + size * spec.referencePointLocalRelative;

                spec.referencePointLatLong = { refLongLat[1], refLongLat[0] };
            }
            specToUi(spec);
        });
    }

    auto originAction = addRefAction(refToOrigin);
    connect(originAction, &QAction::triggered, [this] ()
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
        spec.referencePointLocalRelative = m_dataObject->bounds().convertTo<2>().relativeOriginPosition();
        specToUi(spec);
    });

    auto adjustLocalToGeoAction = addRefAction(refAdjustLocalToGeo);
    connect(adjustLocalToGeoAction, &QAction::triggered, [this] ()
    {
        if (!m_dataObject)
        {
            return;
        }

        auto spec = specFromUi();
        if (spec.type != CoordinateSystemType::geographic)
        {
            return;
        }

        auto && min = m_dataObject->bounds().convertTo<2>().min();
        auto && size = m_dataObject->bounds().convertTo<2>().componentSize();
        const auto refLongLat = vtkVector2d{ spec.referencePointLatLong[1], spec.referencePointLatLong[0] };

        spec.referencePointLocalRelative = (refLongLat - min) / size;
        specToUi(spec);
    });

    auto adjustGeoToLocalAction = addRefAction(refAdjustGeoToLocal);
    connect(adjustGeoToLocalAction, &QAction::triggered, [this] ()
    {
        if (!m_dataObject)
        {
            return;
        }

        auto spec = specFromUi();
        if (spec.type != CoordinateSystemType::geographic)
        {
            return;
        }

        auto && min = m_dataObject->bounds().convertTo<2>().min();
        auto && size = m_dataObject->bounds().convertTo<2>().componentSize();
        const auto refLongLat = min + size * spec.referencePointLocalRelative;

        spec.referencePointLatLong = { refLongLat[1], refLongLat[0] };
        specToUi(spec);
    });
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
