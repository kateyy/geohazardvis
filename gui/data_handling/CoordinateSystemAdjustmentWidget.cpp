/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CoordinateSystemAdjustmentWidget.h"
#include "ui_CoordinateSystemAdjustmentWidget.h"

#include <cassert>
#include <map>

#include <QMenu>
#include <QMessageBox>

#include <core/CoordinateSystems.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/mathhelper.h>
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
    RefPointQuicklyNumValues
};

const QString & refPointQuicklyName(const RefPointQuickly actionType)
{
    static const std::map<RefPointQuickly, QString> map = {
        { refUnsupported, "Set coordinate system type first..." },
        { refToCenter, "Center" },
        { refToNorthEast, "North East" },
        { refToNorthWest, "North West" },
        { refToSouthEast, "South East" },
        { refToSouthWest, "South West" },
    };

    auto && it = map.find(actionType);
    assert(it != map.end());
    return it->second;
}

vtkVector2d refPointQuicklyToRelativePos(const RefPointQuickly actionType)
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

bool refPointQuicklySupports(const RefPointQuickly actionType,
    const CoordinateSystemType coordinateSystemType)
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
        return coordinateSystemType == CoordinateSystemType::geographic;
    default:
        assert(false);
        return false;
    }
}

}


CoordinateSystemAdjustmentWidget::CoordinateSystemAdjustmentWidget(
    QWidget * parent, const Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui{ std::make_unique<Ui_CoordinateSystemAdjustmentWidget>() }
    , m_autoSetReferencePointMenu{ std::make_unique<QMenu>() }
{
    m_ui->setupUi(this);

    m_ui->coordinateSystemTypeCombo->clear();

    for (auto it : CoordinateSystemType::typeToStringMap())
    {
        m_ui->coordinateSystemTypeCombo->addItem(it.second);
    }

    connect(m_ui->referencePointGlobalCheckBox, &QCheckBox::toggled,
        m_ui->refPointGlobalWidget, &QWidget::setEnabled);

    setupQuickSetActions();

    connect(m_ui->autoSetReferencePointButton, &QPushButton::clicked, [this] ()
    {
        m_autoSetReferencePointMenu->exec(
            m_ui->autoSetReferencePointButton->mapToGlobal(
                { 0, m_ui->autoSetReferencePointButton->height() }));
    });

    m_ui->refPointGlobalWidget->setEnabled(m_ui->referencePointGlobalCheckBox->isChecked());

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

    disconnect(m_ui->coordinateSystemTypeCombo, &QComboBox::currentTextChanged,
        this, &CoordinateSystemAdjustmentWidget::updateInfoText);

    if (!dataObject)
    {
        return;
    }

    m_dataObject = dataObject;
    m_ui->dataSetNameEdit->setText(dataObject->name());
    m_ui->dataSetTypeEdit->setText(dataObject->dataTypeName());

    specToUi(dataObject->coordinateSystem());

    updateInfoText();

    connect(m_ui->coordinateSystemTypeCombo, &QComboBox::currentTextChanged,
        this, &CoordinateSystemAdjustmentWidget::updateInfoText);
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

    bool anyRefPointQuicklySupported = false;
    for (int i = 0; i < RefPointQuicklyNumValues; ++i)
    {
        const auto actionType = static_cast<RefPointQuickly>(i);
        const bool supported = refPointQuicklySupports(actionType, currentType);
        anyRefPointQuicklySupported |= supported;
        m_refPointQuickSetActions[i]->setVisible(supported);
    }
    m_ui->autoSetReferencePointButton->setEnabled(anyRefPointQuicklySupported);

    // Geographic systems always have to use degrees as unit, metric systems are in *m;
    m_ui->unitEdit->setReadOnly(currentType == CoordinateSystemType::geographic);
    switch (currentType.value)
    {
    case CoordinateSystemType::geographic:
        m_ui->unitEdit->setPlaceholderText(QChar(0xb0));    // degree
        m_ui->unitEdit->setText("");
        break;
    case CoordinateSystemType::metricGlobal:
    case CoordinateSystemType::metricLocal:
        m_ui->unitEdit->setPlaceholderText("km");
        break;
    case CoordinateSystemType::other:
    case CoordinateSystemType::unspecified:
        m_ui->unitEdit->setPlaceholderText("");
        break;
    }
}

void CoordinateSystemAdjustmentWidget::finish()
{
    if (!m_dataObject)
    {
        reject();
    }

    const auto spec = specFromUi();

    if (!spec.isValid())
    {
        auto infoText = QString("Coordinate System Specification is not valid.");
        if (spec.type == CoordinateSystemType::metricGlobal
            || spec.type == CoordinateSystemType::metricLocal)
        {
            infoText += " The unit of measurement is not valid or unsupported.";
        }
        QMessageBox::warning(this, windowTitle(), infoText);
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

            if (spec.type != CoordinateSystemType::geographic)
            {
                return;
            }

            auto && min = m_dataObject->bounds().convertTo<2>().min();  // x: longitudes, y: latitudes
            auto && size = m_dataObject->bounds().convertTo<2>().componentSize();

            const auto refLongLat = min + size * refPointQuicklyToRelativePos(actionType);
            spec.referencePointLatLong = { refLongLat[1], refLongLat[0] };

            specToUi(spec);
        });
    }
}

ReferencedCoordinateSystemSpecification CoordinateSystemAdjustmentWidget::specFromUi() const
{
    ReferencedCoordinateSystemSpecification spec;

    spec.type.fromString(m_ui->coordinateSystemTypeCombo->currentText());

    spec.geographicSystem = m_ui->geoCoordSystemEdit->text();
    spec.globalMetricSystem = m_ui->metricCoordSystemEdit->text();

    auto && uiUnit = m_ui->unitEdit->text();
    if (spec.type != CoordinateSystemType::geographic)
    {
        auto && unit = uiUnit.isEmpty() ? m_ui->unitEdit->placeholderText() : uiUnit;
        if (mathhelper::isValidMetricUnit(unit))
        {
            spec.unitOfMeasurement = unit;
        }
    }

    if (m_ui->referencePointGlobalCheckBox->isChecked())
    {
        spec.referencePointLatLong.SetX(m_ui->refGlobalLatitudeSpinBox->value());
        spec.referencePointLatLong.SetY(m_ui->refGlobalLongitudeSpinBox->value());
    }
    else
    {
        uninitializeVector(spec.referencePointLatLong);
    }

    return spec;
}

void CoordinateSystemAdjustmentWidget::specToUi(const ReferencedCoordinateSystemSpecification & spec)
{
    m_ui->coordinateSystemTypeCombo->setCurrentText(spec.type.toString());

    //spec.geographicSystem // not implemented yet
    //spec.globalMetricSystem // not implemented yet

    m_ui->unitEdit->setText(spec.unitOfMeasurement);

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
}
