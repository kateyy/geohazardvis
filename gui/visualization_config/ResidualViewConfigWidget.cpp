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

#include "ResidualViewConfigWidget.h"
#include "ui_ResidualViewConfigWidget.h"

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>

#include <core/DataSetHandler.h>
#include <core/utility/mathhelper.h>
#include <core/utility/qthelper.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/data_view/RendererImplementationResidual.h>


ResidualViewConfigWidget::ResidualViewConfigWidget(QWidget * parent)
    : QWidget(parent)
    , m_ui{ std::make_unique<Ui_ResidualViewConfigWidget>() }
{
    m_ui->setupUi(this);

    connect(m_ui->observationCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualViewConfigWidget::updateObservationFromUi);
    connect(m_ui->modelCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualViewConfigWidget::updateModelFromUi);
}

ResidualViewConfigWidget::~ResidualViewConfigWidget() = default;

void ResidualViewConfigWidget::setCurrentView(ResidualVerificationView * view)
{
    m_currentView = view;

    disconnectAll(m_viewConnects);

    setEnabled(m_currentView != nullptr);
    if (!m_currentView)
    {
        m_ui->observationCombo->clear();
        m_ui->modelCombo->clear();
        return;
    }

    auto indexToGeometrySource = [] (int index)
    {
        return index == 0
            ? ResidualVerificationView::InputData::observation
            : ResidualVerificationView::InputData::model;
    };
    auto geometrySourceToIndex = [indexToGeometrySource] (ResidualVerificationView::InputData source)
    {
        return indexToGeometrySource(0) == source ? 0 : 1;
    };

    connect(&view->dataSetHandler(), &DataSetHandler::dataObjectsChanged, this, &ResidualViewConfigWidget::updateComboBoxes);

    m_ui->residualGeomtryCombo->setCurrentIndex(geometrySourceToIndex(view->residualGeometrySource()));
    const auto comboIndexChanged = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    m_viewConnects.emplace_back(connect(m_ui->residualGeomtryCombo, comboIndexChanged,
    [view, indexToGeometrySource] (int index)
    {
        view->setResidualGeometrySource(indexToGeometrySource(index));
    }));
    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::residualGeometrySourceChanged,
    [this, geometrySourceToIndex] (ResidualVerificationView::InputData source)
    {
        QSignalBlocker signalBlocker(m_ui->residualGeomtryCombo);
        m_ui->residualGeomtryCombo->setCurrentIndex(geometrySourceToIndex(source));
    }));

    const double incidenceAngleDeg = view->losIncidenceAngleDegrees();
    const double satelliteHeadingDeg = view->losSatelliteHeadingDegrees();

    auto uiSetLos = [this] (const double incidenceAngleDeg, const double satelliteHeadingDeg)
    {
        const auto signalBlockers = {
            QSignalBlocker(m_ui->losX), QSignalBlocker(m_ui->losY), QSignalBlocker(m_ui->losZ),
            QSignalBlocker(m_ui->satelliteAlphaSpinBox), QSignalBlocker(m_ui->satelliteThetaSpinBox)
        };

        m_ui->satelliteAlphaSpinBox->setValue(incidenceAngleDeg);
        m_ui->satelliteThetaSpinBox->setValue(satelliteHeadingDeg);

        // Modify the UI LOS-vector only if it's input is disabled. If the user is currently
        // entering the LOS-vector directly, changing the UI values would just kill usability.
        // After every changed value, the whole vector would be normalized, leading to simply
        // wrong/unintended values.
        if (!m_ui->losRadioButton->isChecked())
        {
            const auto los = mathhelper::satelliteAnglesToLOSVector(incidenceAngleDeg, satelliteHeadingDeg);
            m_ui->losX->setValue(los.GetX());
            m_ui->losY->setValue(los.GetY());
            m_ui->losZ->setValue(los.GetZ());
        }
    };
    uiSetLos(incidenceAngleDeg, satelliteHeadingDeg);

    auto viewSetLos = [this, view] ()
    {
        // Fetch LOS from angles or LOS-vector. If computing the angles from the vector, fetch the
        // current angles first, as the LOS-vector may not express an incidence angle (0, 0, 1).
        double inc = m_ui->satelliteAlphaSpinBox->value();
        double heading = m_ui->satelliteThetaSpinBox->value();
        if (m_ui->losRadioButton->isChecked())
        {
            vtkVector3d los = {
                m_ui->losX->value(),
                m_ui->losY->value(),
                m_ui->losZ->value(),
            };
            mathhelper::losVectorToSatelliteAngles(los, inc, heading);
        }

        if (view->losIncidenceAngleDegrees() == inc
            && view->losSatelliteHeadingDegrees() == heading)
        {
            return;
        }

        view->setDeformationLineOfSight(inc, heading);
    };

    m_viewConnects.emplace_back(connect(m_ui->losX, &QDoubleSpinBox::editingFinished, viewSetLos));
    m_viewConnects.emplace_back(connect(m_ui->losY, &QDoubleSpinBox::editingFinished, viewSetLos));
    m_viewConnects.emplace_back(connect(m_ui->losZ, &QDoubleSpinBox::editingFinished, viewSetLos));
    m_viewConnects.emplace_back(connect(m_ui->satelliteAlphaSpinBox, &QDoubleSpinBox::editingFinished, viewSetLos));
    m_viewConnects.emplace_back(connect(m_ui->satelliteThetaSpinBox, &QDoubleSpinBox::editingFinished, viewSetLos));

    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::inputDataChanged,
        [this, view] ()
    {
        const QSignalBlocker observationSignalBlocker(m_ui->observationCombo);
        const QSignalBlocker modelSignalBlocker(m_ui->modelCombo);

        m_ui->observationCombo->setCurrentIndex(
            m_ui->observationCombo->findData(dataObjectPtrToVariant(view->observationData())));
        m_ui->modelCombo->setCurrentIndex(
            m_ui->modelCombo->findData(dataObjectPtrToVariant(view->modelData())));
    }));

    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::lineOfSightChanged, uiSetLos));

    const auto qSpinBoxValueChanged = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    m_viewConnects.emplace_back(connect(m_ui->observationScale, qSpinBoxValueChanged,
        view, &ResidualVerificationView::setObservationUnitDecimalExponent));
    m_viewConnects.emplace_back(connect(m_ui->modelScale, qSpinBoxValueChanged,
        view, &ResidualVerificationView::setModelUnitDecimalExponent));

    m_viewConnects.emplace_back(connect(view, &ResidualVerificationView::unitDecimalExponentsChanged,
    [this] (int o, int m)
    {
        const QSignalBlocker observationSignalBlocker(m_ui->observationScale);
        const QSignalBlocker modelSignalBlocker(m_ui->modelScale);

        m_ui->observationScale->setValue(o);
        m_ui->modelScale->setValue(m);
    }));

    m_ui->showModelCheckBox->setChecked(view->implementationResidual().showModel());
    m_viewConnects.emplace_back(connect(m_ui->showModelCheckBox, &QCheckBox::toggled,
    [view] (const bool checked)
    {
        view->implementationResidual().setShowModel(checked);
        view->render();
    }));

    m_viewConnects.emplace_back(connect(m_ui->updateButton, &QAbstractButton::pressed, view, &ResidualVerificationView::updateResidual));

    updateComboBoxes();
}

void ResidualViewConfigWidget::updateComboBoxes()
{
    const QSignalBlocker observationSignalBlocker(m_ui->observationCombo);
    const QSignalBlocker modelSignalBlocker(m_ui->modelCombo);

    m_ui->observationCombo->clear();
    m_ui->modelCombo->clear();


    if (!m_currentView)
    {
        return;
    }

    m_ui->observationCombo->addItem("", 0u);
    m_ui->modelCombo->addItem("", 0u);

    for (auto * dataObject : m_currentView->dataSetHandler().dataSets())
    {
        if (dataObject == m_currentView->residualData())
        {
            continue;
        }

        if (![dataObject] ()
            {
                if (dynamic_cast<ImageDataObject *>(dataObject))
                {
                    return true;
                }

                if (auto genericPoly = dynamic_cast<GenericPolyDataObject *>(dataObject))
                {
                    if (auto poly = dynamic_cast<PolyDataObject *>(genericPoly))
                    {
                        return poly->is2p5D();
                    }
                    return true;
                }

                return false;
            }())
        {
            continue;
        }

        const auto ptrData = dataObjectPtrToVariant(dataObject);
        m_ui->observationCombo->addItem(dataObject->name(), ptrData);
        m_ui->modelCombo->addItem(dataObject->name(), ptrData);
    }

    const QString observationName = m_currentView->observationData() ? m_currentView->observationData()->name() : "";
    const QString modelName = m_currentView->modelData() ? m_currentView->modelData()->name() : "";
    m_ui->observationCombo->setCurrentText(observationName);
    m_ui->modelCombo->setCurrentText(modelName);
}

void ResidualViewConfigWidget::updateObservationFromUi(int index)
{
    if (!m_currentView)
    {
        return;
    }

    auto dataObject = variantToDataObjectPtr(m_ui->observationCombo->itemData(index));
    m_currentView->setObservationData(dataObject);
}

void ResidualViewConfigWidget::updateModelFromUi(int index)
{
    if (!m_currentView)
    {
        return;
    }

    auto dataObject = variantToDataObjectPtr(m_ui->modelCombo->itemData(index));
    m_currentView->setModelData(dataObject);
}
