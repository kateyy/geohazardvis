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

#include <gui/data_handling/DEMWidget.h>
#include "ui_DEMWidget.h"

#include <QDebug>
#include <QMessageBox>

#include <vtkExecutive.h>
#include <vtkPassArrays.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/DataSetHandler.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/filters/DEMToTopographyMesh.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/rendered_data/RenderedImageData.h>
#include <core/utility/DataSetFilter.h>
#include <core/utility/qthelper.h>
#include <core/utility/type_traits.h>
#include <core/utility/vtkvectorhelper.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementation.h>


DEMWidget::DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer, QWidget * parent, Qt::WindowFlags f,
    t_filterFunction topoDataSetFilter,
    t_filterFunction demDataSetFilter)
    : DockableWidget(parent, f)
    , m_dataMapping{ dataMapping }
    , m_ui{ std::make_unique<Ui_DEMWidget>() }
    , m_topographyMeshes{ std::make_unique<DataSetFilter>(dataMapping.dataSetHandler()) }
    , m_dems{ std::make_unique<DataSetFilter>(dataMapping.dataSetHandler()) }
    , m_demUnitDecimalExponent{ 0 }
    , m_meshParametersInvalid{ true }
    , m_previewRebuildRequired{ true }
    , m_demToTopoFilter{ vtkSmartPointer<DEMToTopographyMesh>::New() }
    , m_previewRenderer{ previewRenderer }
    , m_dataPreview{ nullptr }
    , m_lastPreviewedDEM{ nullptr }
    , m_demSelection{ nullptr }
    , m_lastPreviewedTopo{ nullptr }
    , m_topoTemplateSelection{ nullptr }
{
    m_ui->setupUi(this);

    setupPipeline();

    const auto updateComboBox = [this] (QComboBox * comboBoxPtr, const QList<DataObject *> & filteredList, bool updateDEM)
    {
        auto & comboBox = *comboBoxPtr;
        QSignalBlocker signalBlocker(comboBox);

        const auto lastSelection = variantToDataObjectPtr(comboBox.currentData());

        comboBox.clear();
        comboBox.addItem("");
        for (auto p : filteredList)
        {
            comboBox.addItem(p->name(), dataObjectPtrToVariant(p));
        }

        if (lastSelection)
        {
            int restoredSelectionIndex = filteredList.indexOf(lastSelection);
            if (restoredSelectionIndex != -1)
            {
                comboBox.setCurrentIndex(restoredSelectionIndex + 1);
                return;
            }
        }

        comboBox.setCurrentIndex(0);

        if (updateDEM)
        {
            updateTargetCoordsComboForCurrentDEM();
        }

        updatePreview();
    };

    connect(m_topographyMeshes.get(), &DataSetFilter::listChanged,
        std::bind(updateComboBox, m_ui->topoTemplateCombo, std::placeholders::_1, false));
    connect(m_dems.get(), &DataSetFilter::listChanged,
        std::bind(updateComboBox, m_ui->demCombo, std::placeholders::_1, true));

    m_topographyMeshes->setFilterFunction(topoDataSetFilter);
    m_dems->setFilterFunction(demDataSetFilter);

    resetParametersForCurrentInputs();

    const auto comboIndexChangedSignal = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(m_ui->topoTemplateCombo, comboIndexChangedSignal, this, &DEMWidget::updatePreview);
    connect(m_ui->demCombo, comboIndexChangedSignal, [this] ()
    {
        updateTargetCoordsComboForCurrentDEM();
        updatePreview();
    });

    const auto applyDEMUnitScale = [this] () -> bool
    {
        const auto newVal = m_ui->demUnitScaleSpinBox->value();
        if (newVal == m_demUnitDecimalExponent)
        {
            return false;
        }

        m_demUnitDecimalExponent = newVal;

        m_demToTopoFilter->SetElevationScaleFactor(std::pow(10, newVal));

        return true;
    };

    applyDEMUnitScale();

    connect(m_ui->demUnitScaleSpinBox, &QSpinBox::editingFinished, [this, applyDEMUnitScale] ()
    {
        if (!applyDEMUnitScale() || !m_dataPreview || !m_previewRenderer)
        {
            return;
        }

        updatePipeline();

        // Setting a wrong DEM unit can really break the view settings, so reset the camera here.
        // This requires the updated pipeline
        if (m_previewRenderer)  // always check for concurrent input events
        {
            m_previewRenderer->implementation().resetCamera(true, 0);
        }
    });

    connect(m_ui->targetCoordinateSystemComboBox, comboIndexChangedSignal, [this] ()
    {
        // setup pipeline to use the selected coordinate system
        m_meshParametersInvalid = true;

        updatePreview();
    });

    connect(m_ui->radiusMatchingButton, &QAbstractButton::clicked, [this] ()
    {
        matchTopoMeshRadius();

        if (m_previewRenderer)
        {
            updatePipeline();
            m_previewRenderer->render();
        }
    });
    connect(m_ui->templateCenterButton, &QAbstractButton::clicked, [this] ()
    {
        centerTopoMesh();

        if (m_previewRenderer)
        {
            updatePipeline();
            m_previewRenderer->render();
        }
    });

    // NOTE: QAbstractSpinBox::editingFinished is ONLY emitted when the widget loses focus or when
    // enter is pressed, but NOT when changing its value via ->setValue();
    // only valueChanged(T/QString) is emitted in this case
    const auto valueDChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    auto updateForChangeMeshParameters = [this] ()
    {
        applyUIChanges();

        if (m_previewRenderer)
        {
            updatePipeline();
            m_previewRenderer->render();
        }
    };
    connect(m_ui->topographyRadiusSpinBox, valueDChanged, updateForChangeMeshParameters);
    connect(m_ui->topographyCenterXSpinBox, valueDChanged, updateForChangeMeshParameters);
    connect(m_ui->topographyCenterYSpinBox, valueDChanged, updateForChangeMeshParameters);

    connect(m_ui->showPreviewButton, &QAbstractButton::clicked, this, &DEMWidget::showPreview);
    connect(m_ui->saveButton, &QAbstractButton::clicked, this, &DEMWidget::saveAndClose);
    connect(m_ui->cancelButton, &QAbstractButton::clicked, this, &DEMWidget::close);
}

DEMWidget::DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer, QWidget * parent, Qt::WindowFlags f)
    : DEMWidget(dataMapping, previewRenderer, parent, f,
    t_filterFunction([] (DataObject * dataObject, const DataSetHandler &) -> bool
    {
        if (auto poly = dynamic_cast<PolyDataObject *>(dataObject))
        {
            return poly->is2p5D();
        }

        return false;
    }),
    t_filterFunction([] (DataObject * dataObject, const DataSetHandler &) -> bool
    {
        return dynamic_cast<ImageDataObject *>(dataObject) != nullptr;
    }))
{
}

DEMWidget::DEMWidget(PolyDataObject & templateMesh, ImageDataObject & dem,
    DataMapping & dataMapping, AbstractRenderView * previewRenderer,
    QWidget * parent, Qt::WindowFlags f)
    : DEMWidget(dataMapping, previewRenderer, parent, f,
    t_filterFunction([&templateMesh] (DataObject * dataObject, const DataSetHandler &) -> bool
    {
        return dynamic_cast<PolyDataObject *>(dataObject) != nullptr
            && dataObject == &templateMesh;
    }),
    t_filterFunction([&dem] (DataObject * dataObject, const DataSetHandler &) -> bool
    {
        return dynamic_cast<ImageDataObject *>(dataObject) != nullptr
            && dataObject == &dem;
    }))
{
    m_ui->topoTemplateCombo->setEnabled(false);
    m_ui->topoTemplateCombo->setCurrentText(templateMesh.name());
    m_ui->demCombo->setEnabled(false);
    m_ui->demCombo->setCurrentText(dem.name());
}

DEMWidget::~DEMWidget()
{
    signalClosing();

    if (!m_previewRenderer)
    {
        return;
    }

    QList<DataObject *> objectsToRemove;
    auto currentDem = dem();
    auto topo = m_dataPreview.get();
    if (currentDem)
    {
        objectsToRemove << currentDem;
    }

    if (topo)
    {
        objectsToRemove << topo;
    }

    if (m_previewRenderer->dataObjects().toSet() == objectsToRemove.toSet())
    {
        // it only shows our data, so close it
        m_previewRenderer->close();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    else if (!objectsToRemove.isEmpty())
    {
        // remove our data and keep it open
        m_previewRenderer->prepareDeleteData(objectsToRemove);
    }
}

ImageDataObject * DEMWidget::dem()
{
    return static_cast<ImageDataObject *>(
        variantToDataObjectPtr(m_ui->demCombo->currentData()));
}

void DEMWidget::setDEM(ImageDataObject * dem)
{
    const auto lock = m_dems->scopedLock();

    const auto index = m_dems->filteredDataSetList().indexOf(dem);

    if (index != -1)
    {
        m_ui->demCombo->setCurrentIndex(index + 1);
    }
}

PolyDataObject * DEMWidget::meshTemplate()
{
    return static_cast<PolyDataObject *>(
        variantToDataObjectPtr(m_ui->topoTemplateCombo->currentData()));
}

void DEMWidget::setMeshTemplate(PolyDataObject * meshTemplate)
{
    const auto lock = m_topographyMeshes->scopedLock();

    const auto index = m_topographyMeshes->filteredDataSetList().indexOf(meshTemplate);

    if (index != -1)
    {
        m_ui->topoTemplateCombo->setCurrentIndex(index + 1);
    }
}

CoordinateSystemType DEMWidget::targetCoordinateSystem() const
{
    const auto currentSelection = m_ui->targetCoordinateSystemComboBox->currentData();

    return currentSelection.isValid()
        ? CoordinateSystemType::Value(currentSelection.value<std::underlying_type_t<CoordinateSystemType::Value>>())
        : CoordinateSystemType::unspecified;
}

void DEMWidget::setTargetCoordinateSystem(CoordinateSystemType targetCoordinateSystem)
{
    const int i = m_ui->targetCoordinateSystemComboBox->findData(targetCoordinateSystem.value);
    if (i >= 0)
    {
        m_ui->targetCoordinateSystemComboBox->setCurrentIndex(i);
    }
}

double DEMWidget::topoRadius() const
{
    return m_demToTopoFilter->GetTopographyRadius();
}

void DEMWidget::setTopoRadius(double radius)
{
    m_ui->topographyRadiusSpinBox->setValue(radius);
}

const vtkVector2d & DEMWidget::topographyCenterXY() const
{
    return m_demToTopoFilter->GetTopographyShiftXY();
}

void DEMWidget::setTopographyCenterXY(const vtkVector2d & shift)
{
    // execute update code only once for both widgets
    const auto blockers = uiSignalBlockers();

    m_ui->topographyCenterXSpinBox->setValue(shift[0]);
    m_ui->topographyCenterYSpinBox->setValue(shift[1]);

    applyUIChanges();
}

int DEMWidget::demUnitScaleExponent() const
{
    return m_demUnitDecimalExponent;
}

void DEMWidget::setDEMUnitScaleExponent(int exponent)
{
    m_ui->demUnitScaleSpinBox->setValue(exponent);
}

bool DEMWidget::centerTopographyMesh() const
{
    return m_ui->centerOutputTopographyCheckBox->isChecked();
}

void DEMWidget::setCenterTopographyMesh(bool doCenter)
{
    m_ui->centerOutputTopographyCheckBox->setChecked(doCenter);
}

void DEMWidget::showPreview()
{
    if (!m_previewRenderer)
    {
        m_previewRenderer = m_dataMapping.createDefaultRenderViewType();
    }

    updatePreview();

    if (m_previewRenderer)
    {
        m_previewRenderer->setCurrentCoordinateSystem(targetCoordsDEMSpec());

        m_previewRenderer->implementation().resetCamera(true, 0);


        // updating certain rendering parameter might trigger GUI updates, so check if the renderer
        // was closed meanwhile
        if (!m_previewRenderer)
        {
            return;
        }

        configureDEMVisualization();

        if (!m_previewRenderer)
        {
            return;
        }

        configureMeshVisualization();

        if (!m_previewRenderer)
        {
            return;
        }

        m_previewRenderer->render();
    }
}

std::unique_ptr<PolyDataObject> DEMWidget::saveRelease()
{
    setPipelineInputs();
    updatePreviewDataObject();

    if (!m_dataPreview)
    {
        return{};
    }

    updatePipeline();

    vtkSmartPointer<vtkPolyData> surface;

    // For preview visualization, a mesh positioned on the DEM is required.
    // For the final output, the user can request to center the mesh horizontally on the origin
    // (in the topography model's local coordinate system).
    // This is done by output 0 of the DEM-to-topography filter
    if (m_ui->centerOutputTopographyCheckBox->isChecked())
    {
        auto meshCleanup = createMeshCleanupFilter();
        meshCleanup->SetInputConnection(m_demToTopoFilter->GetOutputPort(0));
        if (meshCleanup->GetExecutive()->Update())
        {
            surface = vtkPolyData::SafeDownCast(meshCleanup->GetOutput());
        }
    }
    else
    {
        surface = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->DeepCopy(m_dataPreview->polyDataSet().GetPoints());
        auto polys = vtkSmartPointer<vtkCellArray>::New();
        polys->DeepCopy(m_dataPreview->polyDataSet().GetPolys());
        surface->SetPoints(points);
        surface->SetPolys(polys);
    }

    if (!surface)
    {
        return{};
    }

    auto newDataName = m_ui->newTopoModelName->text();
    if (newDataName.isEmpty())
    {
        newDataName = m_ui->newTopoModelName->placeholderText();
    }
    auto newDataObject = std::make_unique<PolyDataObject>(newDataName, *surface);
    newDataObject->specifyCoordinateSystem(targetCoordsTopoSpec());

    return newDataObject;
}

bool DEMWidget::save()
{
    auto newData = saveRelease();

    if (!m_dataPreview)
    {
        QMessageBox::information(this, "", "Select a surface mesh and a DEM to apply before!");
        return false;
    }

    if (!newData)
    {
        QMessageBox::warning(this, "", "Could not create a topography mesh. Please check if the input data sets are valid!");
        return false;
    }

    m_dataMapping.dataSetHandler().takeData(std::move(newData));

    return true;
}

void DEMWidget::saveAndClose()
{
    auto topoLock = m_topographyMeshes->scopedLock();
    auto demLock = m_dems->scopedLock();

    if (save())
    {
        close();
        return;
    }

    topoLock.discardFurtherUpdates();
    demLock.discardFurtherUpdates();
}

void DEMWidget::matchTopoMeshRadius()
{
    if (!currentDEMChecked())
    {
        return;
    }

    const auto maxRadius = m_demToTopoFilter->GetValidRadiusRange().max();
    const auto currentRadius = m_ui->topographyRadiusSpinBox->value();

    if (currentRadius == maxRadius)
    {
        return;
    }

    const auto blockers = uiSignalBlockers();

    m_ui->topographyRadiusSpinBox->setValue(maxRadius);

    applyUIChanges();
}

void DEMWidget::centerTopoMesh()
{
    if (!currentDEMChecked())
    {
        return;
    }

    const auto shiftToCenter = m_demToTopoFilter->GetValidShiftRange().center();
    const auto currentShift = vtkVector2d{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value() };

    if (currentShift == shiftToCenter)
    {
        return;
    }

    const auto blockers = uiSignalBlockers();

    m_ui->topographyCenterXSpinBox->setValue(shiftToCenter[0]);
    m_ui->topographyCenterYSpinBox->setValue(shiftToCenter[1]);

    applyUIChanges();
}

void DEMWidget::resetParametersForCurrentInputs()
{
    if (!currentDEMChecked())
    {
        return;
    }

    const auto blockers = uiSignalBlockers();

    m_demToTopoFilter->SetParametersToMatching();

    const auto radius = m_demToTopoFilter->GetTopographyRadius();
    const auto radiusRange = m_demToTopoFilter->GetValidRadiusRange();

    const auto shift = m_demToTopoFilter->GetTopographyShiftXY();
    const auto shiftRange = m_demToTopoFilter->GetValidShiftRange();

    m_ui->topographyRadiusSpinBox->setRange(radiusRange.min(), radiusRange.max());
    m_ui->topographyRadiusSpinBox->setValue(radius);

    m_ui->topographyCenterXSpinBox->setRange(shiftRange.min()[0], shiftRange.max()[0]);
    m_ui->topographyCenterXSpinBox->setValue(shift[0]);

    m_ui->topographyCenterYSpinBox->setRange(shiftRange.min()[1], shiftRange.max()[1]);
    m_ui->topographyCenterYSpinBox->setValue(shift[1]);

    assert(!checkIfRadiusChanged());
    assert(!checkIfShiftChanged());
}

void DEMWidget::setupPipeline()
{
    // remove all attribute arrays (including the "Name" field array)
    auto cleanupMeshAttributes = vtkSmartPointer<vtkPassArrays>::New();
    cleanupMeshAttributes->UseFieldTypesOn();
    cleanupMeshAttributes->AddFieldType(vtkDataObject::AttributeTypes::CELL);
    cleanupMeshAttributes->AddFieldType(vtkDataObject::AttributeTypes::POINT);
    cleanupMeshAttributes->AddFieldType(vtkDataObject::AttributeTypes::FIELD);

    m_meshPipelineStart = cleanupMeshAttributes;

    m_demToTopoFilter->SetInputConnection(1, cleanupMeshAttributes->GetOutputPort());

    m_cleanupOutputMeshAttributes = createMeshCleanupFilter();
    m_cleanupOutputMeshAttributes->SetInputConnection(m_demToTopoFilter->GetOutputPort(1));
}

void DEMWidget::configureDEMVisualization()
{
    assert(m_previewRenderer);

    auto dem = currentDEMChecked();

    if (!dem)
    {
        return;
    }

    auto demRendered = dynamic_cast<RenderedImageData *>(m_previewRenderer->visualizationFor(dem));
    assert(demRendered);
    demRendered->setRepresentation(RenderedData::Representation::both);

    const auto scalarsName = QString::fromUtf8(dem->scalars().GetName());
    demRendered->colorMapping().setCurrentScalarsByName(scalarsName, true);
}

void DEMWidget::configureMeshVisualization()
{
    assert(m_previewRenderer);

    if (!m_previewRenderer)
    {
        // setting current scalars invokes lots of GUI updates
        return;
    }

    auto topo = m_dataPreview.get();
    if (!topo)
    {
        return;
    }

    auto topoVis = m_previewRenderer->visualizationFor(topo);
    assert(topoVis);
    auto topoRendered = dynamic_cast<RenderedPolyData *>(topoVis);
    assert(topoRendered);

    auto prop = topoRendered->createDefaultRenderProperty();
    prop->EdgeVisibilityOn();
    prop->LightingOn();
    prop->SetColor(1, 1, 1);
    prop->SetOpacity(0.7);

    topoRendered->renderProperty()->DeepCopy(prop);
    topoRendered->setRepresentation(RenderedData::Representation::both);
}

void DEMWidget::releasePreviewData()
{
    if (!m_dataPreview)
    {
        return;
    }

    if (m_previewRenderer)
    {
        m_previewRenderer->prepareDeleteData({ m_dataPreview.get() });
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    m_dataPreview.reset();
}

ImageDataObject * DEMWidget::currentDEMChecked()
{
    const int listIndex = m_ui->demCombo->currentIndex() - 1;
    auto current = m_dems->filteredDataSetList().value(listIndex, nullptr);
    assert(!current || dynamic_cast<ImageDataObject *>(current));

    if (current != m_demSelection)
    {
        if (m_demSelection)
        {
            disconnect(m_demSelection, &CoordinateTransformableDataObject::coordinateSystemChanged, this, &DEMWidget::updateForChangedDEMCoordinateSystem);
        }

        m_demSelection = static_cast<ImageDataObject *>(current);
        m_meshParametersInvalid = true;
        m_previewRebuildRequired = true;

        if (m_demSelection)
        {
            connect(m_demSelection, &CoordinateTransformableDataObject::coordinateSystemChanged, this, &DEMWidget::updateForChangedDEMCoordinateSystem);
        }
    }

    return m_demSelection;
}

PolyDataObject * DEMWidget::currentTopoTemplateChecked()
{
    const int listIndex = m_ui->topoTemplateCombo->currentIndex() - 1;
    auto current = m_topographyMeshes->filteredDataSetList().value(listIndex, nullptr);
    assert(!current || dynamic_cast<PolyDataObject *>(current));

    if (current != m_topoTemplateSelection)
    {
        m_topoTemplateSelection = static_cast<PolyDataObject *>(current);
        m_meshParametersInvalid = true;
        m_previewRebuildRequired = true;
    }

    return m_topoTemplateSelection;
}

ReferencedCoordinateSystemSpecification DEMWidget::targetCoordsDEMSpec()
{
    ReferencedCoordinateSystemSpecification targetCoords;

    if (auto currentDEM = dem())
    {
        targetCoords = currentDEM->coordinateSystem();
        targetCoords.type = targetCoordinateSystem();
    }

    return targetCoords;
}

ReferencedCoordinateSystemSpecification DEMWidget::targetCoordsTopoSpec()
{
    ReferencedCoordinateSystemSpecification spec;

    auto currentDEM = dem();
    if (!currentDEM)
    {
        return spec;
    }

    auto demCoordsSpec = targetCoordsDEMSpec();
    spec = demCoordsSpec;

    auto transformedDEM = currentDEM->coordinateTransformedDataSet(demCoordsSpec);
    if (!transformedDEM)
    {
        // DEM's coordinate system is not defined, so the output's should also not be defined.
        return spec;
    }

    const auto demBounds2D = DataBounds(transformedDEM->GetBounds()).convertTo<2>();
    if (demBounds2D.isEmpty())
    {
        return spec;
    }

    // The mesh template is assumed to have its origin (and highest density) at its center.
    // That's why the mesh center is always positioned on the point selected in the UI
    auto && referenceXY = topographyCenterXY();
    spec.referencePointLatLong = { referenceXY.GetY(), referenceXY.GetX() };

    return spec;
}

void DEMWidget::updateForChangedDEMCoordinateSystem()
{
    updateTargetCoordsComboForCurrentDEM();
    updatePreview();
}

void DEMWidget::setPipelineInputs()
{
    auto dem = currentDEMChecked();
    auto topo = currentTopoTemplateChecked();

    if (!m_meshParametersInvalid)
    {
        return;
    }

    QString defaultTitle;
    if (topo && !dem)
    {
        defaultTitle = topo->name();
    }
    else if (dem)
    {
        defaultTitle = dem->name() + (topo ? " (" + topo->name() + ")" : "");
    }
    m_ui->newTopoModelName->setPlaceholderText(defaultTitle);

    if (!dem || !topo)
    {
        return;
    }

    if (auto transformedDEMPort = dem->coordinateTransformedOutputPort(targetCoordsDEMSpec()))
    {
        m_demToTopoFilter->SetInputConnection(transformedDEMPort);
    }
    else    // DEM doesn't support any transformations
    {
        m_demToTopoFilter->SetInputDataObject(dem->dataSet());
    }

    m_meshPipelineStart->SetInputDataObject(topo->dataSet());

    resetParametersForCurrentInputs();

    m_meshParametersInvalid = false;
}

bool DEMWidget::updatePreviewDataObject()
{
    auto demPtr = currentDEMChecked();
    auto topoPtr = currentTopoTemplateChecked();

    if (!m_previewRebuildRequired && demPtr && topoPtr)
    {
        return false;
    }

    // release, and rebuild (if possible) the output data object

    releasePreviewData();

    if (!demPtr || !topoPtr)
    {
        return false;
    }

    m_previewRebuildRequired = false;

    m_cleanupOutputMeshAttributes->UpdateDataObject();

    auto newDataSet = vtkPolyData::SafeDownCast(m_cleanupOutputMeshAttributes->GetOutputDataObject(0));

    if (!newDataSet)
    {
        qWarning() << "DEMWidget: mesh transformation did not succeed";
        return false;
    }

    m_dataPreview = std::make_unique<PolyDataObject>("Topography Preview", *newDataSet);

    return true;
}

void DEMWidget::updatePipeline()
{
    if (!m_dataPreview)
    {
        return;
    }

    // defer UI updates / render events until the whole pipeline is updated
    const ScopedEventDeferral topoDeferral(*m_dataPreview);

    m_cleanupOutputMeshAttributes->Update();

    m_dataPreview->specifyCoordinateSystem(targetCoordsTopoSpec());
}

void DEMWidget::updatePreviewRendererContents()
{
    if (!m_previewRenderer)
    {
        return;
    }

    QList<DataObject *> objectsToRender;
    auto currentDEM = currentDEMChecked();
    if (currentDEM)
    {
        objectsToRender << currentDEM;
    }

    if (m_lastPreviewedDEM && (m_lastPreviewedDEM != currentDEM))
    {
        if (m_dems->filteredDataSetList().contains(m_lastPreviewedDEM))
        {
            m_previewRenderer->hideDataObjects({ m_lastPreviewedDEM });
        }
        else
        {
            m_previewRenderer->prepareDeleteData({ m_lastPreviewedDEM });
        }

        if (!m_previewRenderer) // hideDataObjects() might process render view close events
        {
            return;
        }
    }
    m_lastPreviewedDEM = currentDEM;

    auto currentTopoTemplate = currentTopoTemplateChecked();
    if (m_dataPreview)
    {
        objectsToRender << m_dataPreview.get();
    }
    else
    {
        currentTopoTemplate = nullptr;
    }

    bool resetTopoVis = false;
    if (m_lastPreviewedTopo != currentTopoTemplate)
    {
        resetTopoVis = true;
    }
    m_lastPreviewedTopo = currentTopoTemplate;

    if (objectsToRender.isEmpty())
    {
        return;
    }

    m_previewRenderer->setCurrentCoordinateSystem(targetCoordsDEMSpec());

    QList<DataObject *> incompatible;
    m_previewRenderer->showDataObjects(objectsToRender, incompatible);
    assert(incompatible.isEmpty());

    if (resetTopoVis && m_previewRenderer)
    {
        configureMeshVisualization();
    }
}

void DEMWidget::updatePreview()
{
    setPipelineInputs();

    if (!m_previewRenderer)
    {
        return;
    }

    updatePreviewDataObject();

    updatePipeline();

    updatePreviewRendererContents();
}

void DEMWidget::applyUIChanges()
{
    const auto blockers = uiSignalBlockers();

    auto newRadius = m_ui->topographyRadiusSpinBox->value();
    bool radiusChanged = newRadius != m_demToTopoFilter->GetTopographyRadius();
    auto newShift = vtkVector2d{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value() };
    bool shiftChanged = newShift != m_demToTopoFilter->GetTopographyShiftXY();

    Unqualified<decltype(m_demToTopoFilter->GetValidShiftRange())> newValidShiftRange;
    Unqualified<decltype(m_demToTopoFilter->GetValidRadiusRange())> newValidRadiusRange;

    bool reapplyRadiusToUI = false, reapplyShiftToUI = false;

    // The current radius determines the valid shift range and vice versa.
    // So if one changes, update the other's range and check again if this changes the current value.
    for (int i = 0; i < 3; ++i)
    {
        if (radiusChanged)
        {
            m_demToTopoFilter->SetTopographyRadius(newRadius);
            newValidShiftRange = m_demToTopoFilter->GetValidShiftRange();
            if (!shiftChanged && !newValidShiftRange.contains(newShift))
            {
                // new radius enforces to also change the shift
                shiftChanged = true;
                newShift = newValidShiftRange.clampPoint(newShift);
            }
            reapplyShiftToUI = true;
            radiusChanged = false;
        }

        if (shiftChanged)
        {
            m_demToTopoFilter->SetTopographyShiftXY(newShift);
            newValidRadiusRange = m_demToTopoFilter->GetValidRadiusRange();
            if (!radiusChanged && !newValidRadiusRange.contains(newRadius))
            {
                // new shift enforces to also change the radius
                radiusChanged = true;
                newRadius = newValidRadiusRange.clampValue(newRadius);
            }
            reapplyRadiusToUI = true;
            shiftChanged = false;
        }
    }

    assert(!radiusChanged && !shiftChanged);

    if (reapplyRadiusToUI)
    {
        m_ui->topographyRadiusSpinBox->setValue(newRadius);
        m_ui->topographyRadiusSpinBox->setRange(newValidRadiusRange[0], newValidRadiusRange[1]);
        const auto step = newValidRadiusRange.componentSize() * 0.1;
        m_ui->topographyRadiusSpinBox->setSingleStep(step);
    }

    if (reapplyShiftToUI)
    {
        m_ui->topographyCenterXSpinBox->setValue(newShift[0]);
        m_ui->topographyCenterYSpinBox->setValue(newShift[1]);
        const auto && shiftXRange = newValidShiftRange.extractDimension(0);
        const auto && shiftYRange = newValidShiftRange.extractDimension(1);
        m_ui->topographyCenterXSpinBox->setRange(shiftXRange[0], shiftXRange[1]);
        m_ui->topographyCenterYSpinBox->setRange(shiftYRange[0], shiftYRange[1]);

        const auto step = newValidShiftRange.componentSize() * 0.1;
        m_ui->topographyCenterXSpinBox->setSingleStep(step[0]);
        m_ui->topographyCenterYSpinBox->setSingleStep(step[1]);
    }

    assert(!checkIfRadiusChanged());
    assert(!checkIfShiftChanged());
}

std::vector<QSignalBlocker> DEMWidget::uiSignalBlockers()
{
    std::vector<QSignalBlocker> blockers;

    blockers.emplace_back(*m_ui->topographyRadiusSpinBox);
    blockers.emplace_back(*m_ui->topographyCenterXSpinBox);
    blockers.emplace_back(*m_ui->topographyCenterYSpinBox);

    return blockers;
}

vtkSmartPointer<vtkPassArrays> DEMWidget::createMeshCleanupFilter()
{
    auto cleanup = vtkSmartPointer<vtkPassArrays>::New();
    cleanup->UseFieldTypesOn();
    cleanup->AddFieldType(vtkDataObject::AttributeTypes::CELL);
    cleanup->AddFieldType(vtkDataObject::AttributeTypes::POINT);
    cleanup->AddFieldType(vtkDataObject::AttributeTypes::FIELD);

    return cleanup;
}

bool DEMWidget::checkIfRadiusChanged() const
{
    // assuming UI precision will not be changed
    static const auto d = std::pow(10.0, -m_ui->topographyRadiusSpinBox->decimals());

    const auto uiRadius = m_ui->topographyRadiusSpinBox->value();
    const auto algRadius = m_demToTopoFilter->GetTopographyRadius();

    return std::abs(uiRadius - algRadius) > d;
}

bool DEMWidget::checkIfShiftChanged() const
{
    // assuming UI precision will not be changed
    static const auto d = std::pow(10.0, -m_ui->topographyCenterXSpinBox->decimals());

    const auto uiShift = vtkVector2d{
        m_ui->topographyCenterXSpinBox->value(),
        m_ui->topographyCenterYSpinBox->value()
    };
    const auto algShift = m_demToTopoFilter->GetTopographyShiftXY();
    const auto diff = maxComponent(abs(uiShift - algShift));

    return diff > d;
}

void DEMWidget::updateTargetCoordsComboForCurrentDEM()
{
    auto & combo = *m_ui->targetCoordinateSystemComboBox;
    QSignalBlocker blocker(combo);
    combo.clear();

    if (!dem() || !dem()->coordinateSystem().isValid())
    {
        combo.addItem(CoordinateSystemType(CoordinateSystemType::unspecified).toString(), CoordinateSystemType::unspecified);
        combo.setEnabled(false);
        return;
    }

    combo.setEnabled(true);

    auto & currentDEM = *dem();

    auto && dataSetCoords = currentDEM.coordinateSystem();

    int ownIndex = -1;
    int i = 0;
    for (auto && typeAndName : CoordinateSystemType::typeToStringMap())
    {
        if (typeAndName.first == dataSetCoords.type)
        {
            ownIndex = i;
        }

        // this widget only allows between geographic/global/local in the same reference system
        auto checkCoords = dataSetCoords;
        checkCoords.type = typeAndName.first;
        if (!currentDEM.canTransformTo(checkCoords))
        {
            continue;
        }

        combo.addItem(checkCoords.type.toString(), checkCoords.type.value);
        ++i;
    }

    if (ownIndex == -1)
    {
        combo.insertItem(0, dataSetCoords.type.toString(), dataSetCoords.type.value);
        ownIndex = 0;
    }

    // always use the data set's coordinate system as default
    combo.setCurrentIndex(ownIndex);
}
