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
#include <vtkWarpScalar.h>

#include <core/DataSetHandler.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/utility/vtkvectorhelper.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


DEMWidget::DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer, QWidget * parent, Qt::WindowFlags f)
    : DockableWidget(parent, f)
    , m_dataMapping{ dataMapping }
    , m_ui{ std::make_unique<Ui_DEMWidget>() }
    , m_dataPreview{ nullptr }
    , m_demUnitDecimalExponent{ 0.0 }
    , m_topoRadius{ 1.0 }
    , m_topoShiftXY{ 0.0 }
    , m_previewRenderer{ previewRenderer }
    , m_topoRebuildRequired{ true }
    , m_lastDemSelection{ nullptr }
    , m_lastTopoTemplateSelection{ nullptr }
    , m_lastTopoTemplateRadius{ std::nan(nullptr) }
{
    m_ui->setupUi(this);

    updateAvailableDataSets();

    setupPipeline();

    resetParametersForCurrentInputs();

    connect(m_ui->topoTemplateCombo, &QComboBox::currentTextChanged, this, &DEMWidget::rebuildTopoPreviewData);
    connect(m_ui->demCombo, &QComboBox::currentTextChanged, this, &DEMWidget::rebuildTopoPreviewData);

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

        {
            // block rendering while updating the data set
            ScopedEventDeferral eventDeferral(*m_dataPreview);
            m_pipelineEnd->Update();
        }
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

    connect(&dataMapping.dataSetHandler(), &DataSetHandler::dataObjectsChanged, this, &DEMWidget::updateAvailableDataSets);
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
    rebuildTopoPreviewData();

    auto dem = currentDEM();
    auto topo = m_dataPreview.get();

    if (!dem || !topo)
    {
        return;
    }
    // this is required when the user changed parameters before showing the preview
    m_pipelineEnd->Update();


    const auto previewData = QList<DataObject *>{ dem, topo };

    if (!m_previewRenderer)
    {
        m_previewRenderer = m_dataMapping.openInRenderView(previewData);
    }
    else
    {
        QList<DataObject *> incompatible;
        m_previewRenderer->showDataObjects(previewData, incompatible);
        assert(incompatible.isEmpty());
    }

    if (!m_previewRenderer)
    {
        return;
    }

    m_previewRenderer->implementation().resetCamera(true, 0);

    configureVisualizations();
}

bool DEMWidget::save()
{
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

    auto newData = std::make_unique<PolyDataObject>(m_ui->newTopoModelName->text(), *surface);
    m_dataMapping.dataSetHandler().takeData(std::move(newData));

    return true;
}

void DEMWidget::saveAndClose()
{
    disconnect(&m_dataMapping.dataSetHandler(), &DataSetHandler::dataObjectsChanged,
        this, &DEMWidget::updateAvailableDataSets);

    if (save())
    {
        close();
        return;
    }

    updateAvailableDataSets();

    connect(&m_dataMapping.dataSetHandler(), &DataSetHandler::dataObjectsChanged,
        this, &DEMWidget::updateAvailableDataSets);
}

void DEMWidget::resetParametersForCurrentInputs()
{
    auto topo = currentTopoTemplate();
    auto dem = currentDEM();
    if (!topo || !dem)
    {
        return;
    }

    m_ui->newTopoModelName->setText(topo->name() + (dem ? " (" + dem->name() + ")" : ""));


    const auto signalBlockers = {
        QSignalBlocker(m_ui->topographyRadiusSpinBox),
        QSignalBlocker(m_ui->topographyCenterXSpinBox),
        QSignalBlocker(m_ui->topographyCenterYSpinBox)
    };

    m_topoRadius = std::nan(nullptr);
    m_topoShiftXY = vtkVector2d{ std::nan(nullptr) };

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

    m_pipelineEnd = warpElevation;
}

void DEMWidget::rebuildTopoPreviewData()
{
    auto topoPtr = currentTopoTemplate();
    auto demPtr = currentDEM();

    if (!m_topoRebuildRequired)
    {
        return;
    }

    if (!demPtr || !topoPtr)
    {
        return;
    }

    resetParametersForCurrentInputs();

    auto & dem = *demPtr;
    auto & topo = *topoPtr;

    m_topoRebuildRequired = false;

    if (m_dataPreview && m_previewRenderer)
    {
        m_previewRenderer->prepareDeleteData({ m_dataPreview.get() });
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    bool needToResetCamera = true;

    m_demPipelineStart->SetInputDataObject(dem.dataSet());
    assert(&dem.scalars());

    const auto newDEMBounds = DataBounds(dem.bounds());
    needToResetCamera = newDEMBounds != m_previousDEMBounds;
    m_previousDEMBounds = newDEMBounds;
    
    m_meshPipelineStart->SetInputDataObject(topo.dataSet());

    // setup default parameters
    centerTopoMesh();
    matchTopoMeshRadius();

    
    m_pipelineEnd->Update();

    auto newDataSet = vtkPolyData::SafeDownCast(m_pipelineEnd->GetOutputDataObject(0));

    if (!newDataSet)
    {
        qDebug() << "DEMWidget: mesh transformation did not succeed";
        return;
    }

    m_dataPreview = std::make_unique<PolyDataObject>("Topography Preview", *newDataSet);

    if (!m_previewRenderer)
    {
        return;
    }

    QList<DataObject *> incompatible;
    m_previewRenderer->showDataObjects({ m_dataPreview.get() }, incompatible);
    assert(incompatible.isEmpty());

    if (!m_previewRenderer) // showDataObjects() might process render view close events
    {
        return;
    }

    if (needToResetCamera)
    {
        m_previewRenderer->implementation().resetCamera(true, 0);
    }

    configureVisualizations();

    m_previewRenderer->render();
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

        assert(dynamic_cast<RendererImplementationBase3D *>(&m_previewRenderer->implementation()));
        auto & impl = static_cast<RendererImplementationBase3D &>(m_previewRenderer->implementation());
        impl.colorMapping(0)->setCurrentScalarsByName(scalarsName);
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
        topoRendered->mainActor()->GetProperty()->EdgeVisibilityOff();
        topoRendered->mainActor()->GetProperty()->LightingOn();
        topoRendered->mainActor()->GetProperty()->SetColor(1, 1, 1);
        topoRendered->mainActor()->GetProperty()->SetOpacity(0.7);
    }
}

ImageDataObject * DEMWidget::currentDEM()
{
    auto current = m_dems.value(m_ui->demCombo->currentIndex(), nullptr);

    if (current != m_lastDemSelection)
    {
        m_lastDemSelection = current;
        m_topoRebuildRequired = true;
    }

    return m_lastDemSelection;
}

PolyDataObject * DEMWidget::currentTopoTemplate()
{
    auto current = m_topographyMeshes.value(m_ui->topoTemplateCombo->currentIndex(), nullptr);
    
    if (current != m_lastTopoTemplateSelection)
    {
        m_lastTopoTemplateSelection = current;
        m_lastTopoTemplateRadius = std::nan(nullptr);
        m_topoRebuildRequired = true;
    }

    return m_lastTopoTemplateSelection;
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

void DEMWidget::updateAvailableDataSets()
{
    auto signalBlockers = {
        QSignalBlocker(m_ui->topoTemplateCombo),
        QSignalBlocker(m_ui->demCombo)
    };
    m_topographyMeshes.clear();
    m_dems.clear();
    m_ui->topoTemplateCombo->clear();
    m_ui->demCombo->clear();

    for (auto dataObject : m_dataMapping.dataSetHandler().dataSets())
    {
        if (auto p = dynamic_cast<PolyDataObject *>(dataObject))
        {
            if (p->is2p5D())
                m_topographyMeshes << p;
        }
        else if (auto i = dynamic_cast<ImageDataObject *>(dataObject))
        {
            if (i->dataSet()->GetPointData()->GetScalars() == nullptr)
            {
                qDebug() << "DEMWidget: no scalars found in image" << i->name();
                continue;
            }
            m_dems << i;
        }
    }

    for (auto p : m_topographyMeshes)
        m_ui->topoTemplateCombo->addItem(p->name());

    for (auto d : m_dems)
        m_ui->demCombo->addItem(d->name());

    int lastDEMIndex = m_dems.indexOf(m_lastDemSelection);
    if (lastDEMIndex >= 0)
    {
        m_ui->demCombo->setCurrentIndex(lastDEMIndex);
    }
    else
    {
        m_previousDEMBounds = {};
    }

    int lastTopoIndex = m_topographyMeshes.indexOf(m_lastTopoTemplateSelection);
    if (lastTopoIndex >= 0)
    {
        m_ui->topoTemplateCombo->setCurrentIndex(lastTopoIndex);
    }

    if (m_previewRenderer && (lastDEMIndex == -1 || lastTopoIndex == -1))
    {
        rebuildTopoPreviewData();
    }
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

void DEMWidget::updateView()
{
    if (!m_previewRenderer)
    {
        return;
    }

    {
        std::vector<ScopedEventDeferral> deferrals;
        if (auto dem = currentDEM())
        {
            deferrals.emplace_back(*dem);
        }
        if (auto topo = m_dataPreview.get())
        {
            deferrals.emplace_back(*topo);
        }

        m_pipelineEnd->Update();
    }

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

    const double newTopoRadius = m_ui->topographyRadiusSpinBox->value();

    {
        const vtkVector2d newTopoShift{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value() };
        if (newTopoRadius == m_topoRadius && newTopoShift == m_topoShiftXY)
        {
            return;
        }
    }

    const auto signalBlockers = {
        QSignalBlocker(m_ui->topographyRadiusSpinBox),
        QSignalBlocker(m_ui->topographyCenterXSpinBox),
        QSignalBlocker(m_ui->topographyCenterYSpinBox)
    };


    const auto demBounds = DataBounds(dem.bounds()).convertTo<2>();
    const auto lowerLeft = demBounds.minRange();
    const auto upperRight = demBounds.maxRange();

    if (newTopoRadius != m_topoRadius)
    {
        // clamp valid center to the DEM bounds reduced by newTopoRadius
        const auto demCenter = demBounds.center();
        const auto mins = min(lowerLeft + newTopoRadius, demCenter);
        const auto maxs = max(upperRight - newTopoRadius, demCenter);

        const auto steps = (maxs - mins) * 0.01;

        m_ui->topographyCenterXSpinBox->setRange(mins[0], maxs[0]);
        m_ui->topographyCenterXSpinBox->setSingleStep(steps[0]);
        m_ui->topographyCenterYSpinBox->setRange(mins[1], maxs[1]);
        m_ui->topographyCenterYSpinBox->setSingleStep(steps[1]);
    }

    // might have change now
    const vtkVector2d newTopoShift2{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value() };

    if (newTopoShift2 != m_topoShiftXY)
    {
        // clamp from topography center to the nearest DEM border
        const auto maxRadius = std::min(
            minComponent(abs(newTopoShift2 - lowerLeft)),
            minComponent(abs(newTopoShift2 - upperRight)));

        const auto minValue = minComponent(vtkVector2d(dem.imageData()->GetSpacing()));

        m_ui->topographyRadiusSpinBox->setRange(minValue, maxRadius);
        m_ui->topographyRadiusSpinBox->setSingleStep((maxRadius - minValue) * 0.01);
    }

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
