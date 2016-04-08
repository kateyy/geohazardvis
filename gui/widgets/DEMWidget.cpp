#include <gui/widgets/DEMWidget.h>
#include "ui_DEMWidget.h"

#include <QDebug>
#include <QMessageBox>

#include <vtkActor.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkPassArrays.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVersionMacros.h>
#include <vtkWarpScalar.h>

#include <core/DataSetHandler.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/utility/DataSetFilter.h>
#include <core/utility/vtkvectorhelper.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


DEMWidget::DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer, QWidget * parent, Qt::WindowFlags f,
    t_filterFunction topoDataSetFilter,
    t_filterFunction demDataSetFilter)
    : DockableWidget(parent, f)
    , m_dataMapping{ dataMapping }
    , m_ui{ std::make_unique<Ui_DEMWidget>() }
    , m_topographyMeshes{ std::make_unique<DataSetFilter>(dataMapping.dataSetHandler()) }
    , m_dems{ std::make_unique<DataSetFilter>(dataMapping.dataSetHandler()) }
    , m_dataPreview{ nullptr }
    , m_demUnitDecimalExponent{ 0.0 }
    , m_topoRadius{ 1.0 }
    , m_topoShiftXY{ 0.0 }
    , m_previewRenderer{ previewRenderer }
    , m_topoRebuildRequired{ true }
    , m_lastPreviewedDEM{ nullptr }
    , m_demSelection{ nullptr }
    , m_topoTemplateSelection{ nullptr }
    , m_lastTopoTemplateRadius{ std::nan(nullptr) }
{
    m_ui->setupUi(this);

    setupPipeline();

    const auto updateComboBox = [this] (QComboBox * _comboBox, const QList<DataObject *> & filteredList)
    {
        auto & comboBox = *_comboBox;
        QSignalBlocker signalBlocker(comboBox);

        comboBox.clear();
        comboBox.addItem("");
        for (auto p : filteredList)
            comboBox.addItem(p->name());

        int restoredSelectionIndex = filteredList.indexOf(m_topoTemplateSelection) + 1;
        if (restoredSelectionIndex > 0)
        {
            comboBox.setCurrentIndex(restoredSelectionIndex);
            return;
        }

        comboBox.setCurrentIndex(0);
        updatePreview();
    };

    connect(m_topographyMeshes.get(), &DataSetFilter::listChanged,
        std::bind(updateComboBox, m_ui->topoTemplateCombo, std::placeholders::_1));
    connect(m_dems.get(), &DataSetFilter::listChanged,
        std::bind(updateComboBox, m_ui->demCombo, std::placeholders::_1));

    m_topographyMeshes->setFilterFunction(topoDataSetFilter);
    m_dems->setFilterFunction(demDataSetFilter);

    resetParametersForCurrentInputs();

    const auto indexChangedSignal = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(m_ui->topoTemplateCombo, indexChangedSignal, this, &DEMWidget::updatePreview);
    connect(m_ui->demCombo, indexChangedSignal, this, &DEMWidget::updatePreview);

    auto applyDEMUnitScale = [this] () -> bool {
        const double newValue = m_ui->demUnitScaleSpinBox->value();
        if (newValue == m_demUnitDecimalExponent)
            return false;

        m_demUnitDecimalExponent = newValue;
        m_demUnitScale->SetScale(std::pow(10, m_demUnitDecimalExponent));

        return true;
    };

    applyDEMUnitScale();

    connect(m_ui->demUnitScaleSpinBox, &QSpinBox::editingFinished, [this, applyDEMUnitScale] () {
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

    connect(m_ui->radiusMatchingButton, &QAbstractButton::clicked, [this] () {
        matchTopoMeshRadius();
        updateView();
    });
    connect(m_ui->templateCenterButton, &QAbstractButton::clicked, [this] () {
        centerTopoMesh();
        updateView();
    });

    connect(m_ui->topographyRadiusSpinBox, &QDoubleSpinBox::editingFinished, this, &DEMWidget::updateTopoUIRanges);
    connect(m_ui->topographyCenterXSpinBox, &QDoubleSpinBox::editingFinished, this, &DEMWidget::updateTopoUIRanges);
    connect(m_ui->topographyCenterYSpinBox, &QDoubleSpinBox::editingFinished, this, &DEMWidget::updateTopoUIRanges);

    connect(m_ui->showPreviewButton, &QAbstractButton::clicked, this, &DEMWidget::showPreview);
    connect(m_ui->saveButton, &QAbstractButton::clicked, this, &DEMWidget::saveAndClose);
    connect(m_ui->cancelButton, &QAbstractButton::clicked, this, &DEMWidget::close);
}

DEMWidget::DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer, QWidget * parent, Qt::WindowFlags f)
    : DEMWidget(dataMapping, previewRenderer, parent, f,
    t_filterFunction([] (DataObject * dataSet, const DataSetHandler &) -> bool
    {
        if (auto poly = dynamic_cast<PolyDataObject *>(dataSet))
        {
            return poly->is2p5D();
        }

        return false;
    }),
    t_filterFunction([] (DataObject * dataSet, const DataSetHandler &) -> bool
    {
        if (auto image = dynamic_cast<ImageDataObject *>(dataSet))
        {
            if (image->dataSet()->GetPointData()->GetScalars())
            {
                return true;
            }
            qDebug() << "DEMWidget: no scalars found in image" << image->name();
        }

        return false;
    }))
{
}

DEMWidget::DEMWidget(PolyDataObject & templateMesh, ImageDataObject & dem,
    DataMapping & dataMapping, AbstractRenderView * previewRenderer,
    QWidget * parent, Qt::WindowFlags f)
    : DEMWidget(dataMapping, previewRenderer, parent, f,
    t_filterFunction([&templateMesh] (DataObject * dataSet, const DataSetHandler &) -> bool
    {
        return dynamic_cast<PolyDataObject *>(dataSet) != nullptr
            && dataSet->name() == templateMesh.name();
    }),
    t_filterFunction([&dem] (DataObject * dataSet, const DataSetHandler &) -> bool
    {
        return dynamic_cast<ImageDataObject *>(dataSet) != nullptr
            && dataSet->name() == dem.name();
    }))
{
    m_ui->topoTemplateCombo->setEnabled(false);
    m_ui->topoTemplateCombo->setCurrentText(templateMesh.name());
    m_ui->demCombo->setEnabled(false);
    m_ui->demCombo->setCurrentText(dem.name());
}

DEMWidget::~DEMWidget()
{
    if (!m_previewRenderer)
    {
        return;
    }

    QList<DataObject *> objectsToHide;
    auto dem = currentDEM();
    auto topo = m_dataPreview.get();
    if (dem)
    {
        objectsToHide << dem;
    }

    if (topo)
    {
        objectsToHide << topo;
    }

    if (m_previewRenderer->dataObjects().toSet() == objectsToHide.toSet())
    {
        // it only shows our data, so close it
        m_previewRenderer->close();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    else
    {
        // remove our data and keep it open
        if (dem)
        {
            m_previewRenderer->hideDataObjects({ dem });
        }
        if (topo)
        {
            m_previewRenderer->prepareDeleteData({ topo });
        }
    }
}

void DEMWidget::showPreview()
{
    forceUpdatePreview();

    if (m_dataPreview && m_previewRenderer)
    {
        resetParametersForCurrentInputs();
        configureVisualizations();
        m_previewRenderer->implementation().resetCamera(true, 0);

        updateView();
    }
}

bool DEMWidget::save()
{
    updateData();

    if (!m_dataPreview)
    {
        QMessageBox::information(this, "", "Select a surface mesh and a DEM to apply before!");
        return false;
    }

    auto surface = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->DeepCopy(m_dataPreview->polyDataSet()->GetPoints());
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    polys->DeepCopy(m_dataPreview->polyDataSet()->GetPolys());
    surface->SetPoints(points);
    surface->SetPolys(polys);

    auto newDataName = m_ui->newTopoModelName->text();
    if (newDataName.isEmpty())
    {
        newDataName = m_ui->newTopoModelName->placeholderText();
    }
    auto newData = std::make_unique<PolyDataObject>(newDataName, *surface);
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

    topoLock.release();
    demLock.release();
}

void DEMWidget::resetParametersForCurrentInputs()
{
    auto topo = currentTopoTemplate();
    auto dem = currentDEM();
    if (!topo || !dem)
    {
        return;
    }

    const auto signalBlockers = {
        QSignalBlocker(m_ui->topographyRadiusSpinBox),
        QSignalBlocker(m_ui->topographyCenterXSpinBox),
        QSignalBlocker(m_ui->topographyCenterYSpinBox)
    };

    centerTopoMesh();
    matchTopoMeshRadius();
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

    m_meshTransform = vtkSmartPointer<vtkTransform>::New();
    auto meshTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    meshTransformFilter->SetTransform(m_meshTransform);
    meshTransformFilter->SetInputConnection(cleanupMeshAttributes->GetOutputPort());

    m_demUnitScale = vtkSmartPointer<vtkImageShiftScale>::New();
    m_demPipelineStart = m_demUnitScale;

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(meshTransformFilter->GetOutputPort());
    probe->SetSourceConnection(m_demUnitScale->GetOutputPort());

    auto warpElevation = vtkSmartPointer<vtkWarpScalar>::New();
    warpElevation->SetInputConnection(probe->GetOutputPort());

    auto cleanupOutputMeshAttributes = vtkSmartPointer<vtkPassArrays>::New();
    cleanupOutputMeshAttributes->UseFieldTypesOn();
    cleanupOutputMeshAttributes->AddFieldType(vtkDataObject::AttributeTypes::CELL);
    cleanupOutputMeshAttributes->AddFieldType(vtkDataObject::AttributeTypes::POINT);
    cleanupOutputMeshAttributes->AddFieldType(vtkDataObject::AttributeTypes::FIELD);
    cleanupOutputMeshAttributes->SetInputConnection(warpElevation->GetOutputPort());

    m_pipelineEnd = cleanupOutputMeshAttributes;
}

void DEMWidget::configureVisualizations()
{
    assert(m_previewRenderer);

    if (auto dem = currentDEM())
    {
        auto demVis = m_previewRenderer->visualizationFor(dem);
        assert(demVis);
        auto demRendered = dynamic_cast<RenderedData *>(demVis);
        assert(demRendered);
        demRendered->setRepresentation(RenderedData::Representation::both);
        
        auto scalarsName = QString::fromUtf8(dem->scalars().GetName());
        demVis->colorMapping().setCurrentScalarsByName(scalarsName);
    }

    if (!m_previewRenderer)
    {
        // setting current scalars invokes lots of GUI updates
        return;
    }

    if (auto topo = m_dataPreview.get())
    {
        auto topoVis = m_previewRenderer->visualizationFor(topo);
        assert(topoVis);
        auto topoRendered = dynamic_cast<RenderedPolyData *>(topoVis);
        assert(topoRendered);
        
        auto prop = topoRendered->createDefaultRenderProperty();
        prop->EdgeVisibilityOff();
        prop->LightingOn();
        prop->SetColor(1, 1, 1);
        prop->SetOpacity(0.7);

        topoRendered->renderProperty()->DeepCopy(prop);
        topoRendered->setRepresentation(RenderedData::Representation::both);

#if VTK_MAJOR_VERSION < 7 || VTK_MINOR_VERSION < 1
        topoRendered->renderProperty()->LightingOn();   // bug: flag not copied in VTK < 7.1
#endif
    }
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

ImageDataObject * DEMWidget::currentDEM()
{
    const int listIndex = m_ui->demCombo->currentIndex() - 1;
    auto current = m_dems->filteredDataSetList().value(listIndex, nullptr);
    assert(!current || dynamic_cast<ImageDataObject *>(current));

    if (current != m_demSelection)
    {
        m_lastPreviewedDEM = m_demSelection;
        m_demSelection = static_cast<ImageDataObject *>(current);
        m_topoRebuildRequired = true;
    }

    return m_demSelection;
}

PolyDataObject * DEMWidget::currentTopoTemplate()
{
    const int listIndex = m_ui->topoTemplateCombo->currentIndex() - 1;
    auto current = m_topographyMeshes->filteredDataSetList().value(listIndex, nullptr);
    assert(!current || dynamic_cast<PolyDataObject *>(current));
    
    if (current != m_topoTemplateSelection)
    {
        m_topoTemplateSelection = static_cast<PolyDataObject *>(current);
        m_lastTopoTemplateRadius = std::nan(nullptr);
        m_topoRebuildRequired = true;
    }

    return m_topoTemplateSelection;
}

double DEMWidget::currentTopoTemplateRadius()
{
    auto topo = currentTopoTemplate();
    if (!topo)
    {
        return std::nan(nullptr);
    }

    if (!std::isnan(m_lastTopoTemplateRadius))
    {
        return m_lastTopoTemplateRadius;
    }

    auto points = topo->polyDataSet()->GetPoints();
    const vtkIdType numPoints = points->GetNumberOfPoints();

    vtkVector3d point;
    double maxDistance = 0.0;

    // assume a template centered around (0, 0, z)
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        topo->polyDataSet()->GetPoints()->GetPoint(i, point.GetData());
        maxDistance = std::max(maxDistance, convert<2>(point).Norm());
    }

    m_lastTopoTemplateRadius = maxDistance;

    return m_lastTopoTemplateRadius;
}

bool DEMWidget::updateData()
{
    auto topoPtr = currentTopoTemplate();
    auto demPtr = currentDEM();

    if (!m_topoRebuildRequired)
    {
        return false;
    }
    
    releasePreviewData();

    if (!demPtr || !topoPtr)
    {
        return false;
    }

    auto & dem = *demPtr;
    auto & topo = *topoPtr;

    m_topoRebuildRequired = false;

    m_demPipelineStart->SetInputDataObject(dem.dataSet());
    assert(&dem.scalars());

    m_meshPipelineStart->SetInputDataObject(topo.dataSet());

    resetParametersForCurrentInputs();
    m_pipelineEnd->Update();

    auto newDataSet = vtkPolyData::SafeDownCast(m_pipelineEnd->GetOutputDataObject(0));

    if (!newDataSet)
    {
        qDebug() << "DEMWidget: mesh transformation did not succeed";
        return false;
    }

    m_dataPreview = std::make_unique<PolyDataObject>("Topography Preview", *newDataSet);

    return true;
}

void DEMWidget::updatePreview()
{
    auto topo = currentTopoTemplate();
    auto dem = currentDEM();
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

    if (m_previewRenderer)
    {
        forceUpdatePreview();
    }
}

void DEMWidget::forceUpdatePreview()
{
    const bool dataWasRebuilt = updateData();

    auto dem = currentDEM();
    auto topo = m_dataPreview.get();

    if (!dem || !topo)
    {
        return;
    }

    const auto previewData = QList<DataObject *>{ dem, topo };

    if (!m_previewRenderer)
    {
        m_previewRenderer = m_dataMapping.openInRenderView(previewData);
    }
    else
    {
        if (m_lastPreviewedDEM)
        {
            m_previewRenderer->hideDataObjects({ m_lastPreviewedDEM });
        }

        QList<DataObject *> incompatible;
        m_previewRenderer->showDataObjects(previewData, incompatible);
        assert(incompatible.isEmpty());
    }

    if (!m_previewRenderer) // showDataObjects() might process render view close events
    {
        return;
    }

    if (dataWasRebuilt)
    {
        m_previewRenderer->implementation().resetCamera(true, 0);

        configureVisualizations();
    }

    m_previewRenderer->render();
}

void DEMWidget::updateMeshTransform()
{
    const auto radius = currentTopoTemplateRadius();

    if (std::isnan(radius))
    {
        return;
    }

    m_meshTransform->Identity();
    m_meshTransform->PostMultiply();

    const auto radiusScale = m_topoRadius / radius;

    m_meshTransform->Scale(radiusScale, radiusScale, 0.0);
    m_meshTransform->Translate(convert<3>(m_topoShiftXY, 0.0).GetData()); // assuming the template is already centered around (0,0,z)
}

void DEMWidget::updatePipeline()
{
    if (!m_dataPreview)
    {
        return;
    }

    // defer UI updates / render events until the whole pipeline is updated
    ScopedEventDeferral deferral(*m_dataPreview);

    m_pipelineEnd->Update();
}

void DEMWidget::updateView()
{
    if (!m_previewRenderer)
    {
        return;
    }

    updatePipeline();

    if (m_previewRenderer)
    {
        m_previewRenderer->render();
    }
}

void DEMWidget::matchTopoMeshRadius()
{
    updateTopoUIRanges();

    const auto currentValue = m_ui->topographyRadiusSpinBox->value();
    const auto maxRadius = m_ui->topographyRadiusSpinBox->maximum();

    if (currentValue == maxRadius)
    {
        return;
    }

    m_topoRadius = maxRadius;

    QSignalBlocker signalBlocker(m_ui->topographyRadiusSpinBox);

    m_ui->topographyRadiusSpinBox->setValue(m_topoRadius);

    updateTopoUIRanges();

    updateMeshTransform();
}

void DEMWidget::centerTopoMesh()
{
    updateTopoUIRanges();

    const auto signalBlockers = {
        QSignalBlocker(m_ui->topographyCenterXSpinBox),
        QSignalBlocker(m_ui->topographyCenterYSpinBox) };

    const vtkVector2d mins {
        m_ui->topographyCenterXSpinBox->minimum(),
        m_ui->topographyCenterYSpinBox->minimum() };
    const vtkVector2d maxs {
        m_ui->topographyCenterXSpinBox->maximum(),
        m_ui->topographyCenterYSpinBox->maximum() };

    const vtkVector2d currentShift {
        m_ui->topographyCenterXSpinBox->value(),
        m_ui->topographyCenterYSpinBox->value() };

    const auto center = maxs * 0.5 + mins * 0.5;

    if (center == currentShift)
    {
        return;
    }

    m_topoShiftXY = center;

    m_ui->topographyCenterXSpinBox->setValue(center[0]);
    m_ui->topographyCenterYSpinBox->setValue(center[1]);

    updateTopoUIRanges();

    updateMeshTransform();
}

void DEMWidget::updateTopoUIRanges()
{
    auto demPtr = currentDEM();
    if (!demPtr)
    {
        return;
    }

    auto & dem = *demPtr;

    const double currentTopoRadius = m_ui->topographyRadiusSpinBox->value();

    const auto signalBlockers = {
        QSignalBlocker(m_ui->topographyRadiusSpinBox),
        QSignalBlocker(m_ui->topographyCenterXSpinBox),
        QSignalBlocker(m_ui->topographyCenterYSpinBox)
    };

    const auto demBounds = DataBounds(dem.bounds()).convertTo<2>();
    const auto lowerLeft = demBounds.minRange();
    const auto upperRight = demBounds.maxRange();

    // clamp valid center to the DEM bounds reduced by currentTopoRadius
    const auto demCenter = demBounds.center();
    const auto centerMins = min(lowerLeft + currentTopoRadius, demCenter);
    const auto centerMaxs = max(upperRight - currentTopoRadius, demCenter);
    const auto steps = (centerMaxs - centerMins) * 0.01;

    m_ui->topographyCenterXSpinBox->setRange(centerMins[0], centerMaxs[0]);
    m_ui->topographyCenterXSpinBox->setSingleStep(steps[0]);
    m_ui->topographyCenterYSpinBox->setRange(centerMins[1], centerMaxs[1]);
    m_ui->topographyCenterYSpinBox->setSingleStep(steps[1]);

    // might have change now
    const vtkVector2d newTopoShift2{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value() };

    // clamp from topography center to the nearest DEM border
    const auto maxValidRadius = std::min(
        minComponent(abs(newTopoShift2 - lowerLeft)),
        minComponent(abs(newTopoShift2 - upperRight)));

    const auto minValidRadius = minComponent(vtkVector2d(dem.imageData()->GetSpacing()));

    m_ui->topographyRadiusSpinBox->setRange(minValidRadius, maxValidRadius);
    m_ui->topographyRadiusSpinBox->setSingleStep((maxValidRadius - minValidRadius) * 0.01);

    // modifying valid value range might change the actual values
    updateForChangedTransformParameters();
}

void DEMWidget::updateForChangedTransformParameters()
{
    const double newTopoRadius{ m_ui->topographyRadiusSpinBox->value() };
    const vtkVector2d newTopoShift{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value() };

    if (newTopoRadius == m_topoRadius && newTopoShift == m_topoShiftXY)
    {
        return;
    }

    m_topoRadius = newTopoRadius;
    m_topoShiftXY = newTopoShift;

    updateMeshTransform();
    updateView();
}
