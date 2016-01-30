#include <gui/widgets/DEMWidget.h>
#include "ui_DEMWidget.h"

#include <QDebug>
#include <QMessageBox>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWarpScalar.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/vtkcamerahelper.h>
#include <core/utility/vtkvectorhelper.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <gui/rendering_interaction/InteractorStyleTerrain.h>
#include <gui/data_view/RendererImplementationBase3D.h>


DEMWidget::DEMWidget(DataSetHandler & dataSetHandler, QWidget * parent, Qt::WindowFlags f)
    : DockableWidget(parent, f)
    , m_dataSetHandler(dataSetHandler)
    , m_ui{ std::make_unique<Ui_DEMWidget>() }
    , m_dataPreview{ nullptr }
    , m_demUnitDecimalExponent{ 0.0 }
    , m_topoRadiusScale{ 1.0 }
    , m_topoShift{ 0.0 }
{
    m_ui->setupUi(this);

    updateAvailableDataSets();

    setupPipeline();

    connect(m_ui->topoTemplateCombo, &QComboBox::currentTextChanged, this, &DEMWidget::rebuildPreview);
    connect(m_ui->demCombo, &QComboBox::currentTextChanged, this, &DEMWidget::rebuildPreview);

    auto applyDEMUnitScale = [this] () {
        const double newValue = m_ui->demUnitScaleSpinBox->value();
        if (newValue == m_demUnitDecimalExponent)
            return;

        m_demUnitDecimalExponent = newValue;
        m_demUnitScale->SetScale(std::pow(10, m_demUnitDecimalExponent));
    };

    applyDEMUnitScale();

    connect(m_ui->demUnitScaleSpinBox, &QSpinBox::editingFinished, [this, applyDEMUnitScale] () {
        applyDEMUnitScale();
        m_pipelineEnd->Update();
        // setting a wrong DEM unit can really break the view settings, so reset the camera here
        m_renderer->ResetCamera();
        updateView();
    });

    connect(m_ui->radiusMatchingButton, &QAbstractButton::clicked, [this] () {
        matchTopoMeshRadius();
        updateView();
    });
    connect(m_ui->templateCenterButton, &QAbstractButton::clicked, [this] () {
        centerTopoMesh();
        updateView();
    });

    auto updateForChangedTransform = [this] () -> void {
        double newTopoScale{ m_ui->topographyRadiusSpinBox->value() };
        vtkVector3d newTopoShift{ m_ui->topographyCenterXSpinBox->value(), m_ui->topographyCenterYSpinBox->value(), 0.0 };

        if ((newTopoScale == m_topoRadiusScale) && (newTopoShift == m_topoShift))
        {
            return;
        }

        m_topoRadiusScale = newTopoScale;
        m_topoShift = newTopoShift;

        updateMeshTransform();
        updateView();
    };

    connect(m_ui->topographyRadiusSpinBox, &QDoubleSpinBox::editingFinished, updateForChangedTransform);
    connect(m_ui->topographyCenterXSpinBox, &QDoubleSpinBox::editingFinished, updateForChangedTransform);
    connect(m_ui->topographyCenterYSpinBox, &QDoubleSpinBox::editingFinished, updateForChangedTransform);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &DEMWidget::saveAndClose);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);


    connect(&m_dataSetHandler, &DataSetHandler::dataObjectsChanged, this, &DEMWidget::updateAvailableDataSets);
}

DEMWidget::~DEMWidget()
{
    if (m_renderer)
    {
        m_renderer->RemoveAllViewProps();
    }
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
    m_dataSetHandler.takeData(std::move(newData));

    return true;
}

void DEMWidget::saveAndClose()
{
    disconnect(&m_dataSetHandler, &DataSetHandler::dataObjectsChanged,
        this, &DEMWidget::updateAvailableDataSets);

    if (save())
        close();

    updateAvailableDataSets();

    connect(&m_dataSetHandler, &DataSetHandler::dataObjectsChanged,
        this, &DEMWidget::updateAvailableDataSets);
}

void DEMWidget::showEvent(QShowEvent * /*event*/)
{
    if (m_renderer)
        return;

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(1, 1, 1);

    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_renderer);

    vtkCamera & camera = *m_renderer->GetActiveCamera();
    camera.SetViewUp(0, 0, 1);
    TerrainCamera::setAzimuth(camera, 0);
    TerrainCamera::setVerticalElevation(camera, 45);

    auto interactorStyle = vtkSmartPointer<InteractorStyleTerrain>::New();
    m_ui->qvtkMain->GetInteractor()->SetInteractorStyle(interactorStyle);
    interactorStyle->SetCurrentRenderer(m_renderer);


    m_axesActor = RendererImplementationBase3D::createAxes();

    rebuildPreview();
}

void DEMWidget::rebuildPreview()
{
    m_renderer->RemoveAllViewProps();
    m_renderedPreview.reset();
    m_dataPreview.reset();

    int surfaceIdx = m_ui->topoTemplateCombo->currentIndex();
    if (surfaceIdx == -1)
        return;

    int demIndex = m_ui->demCombo->currentIndex();
    auto currentDEM = m_dems.value(demIndex, nullptr);
    auto & currentTopo = *m_topographyMeshes[surfaceIdx];

    bool needToResetCamera = true;

    if (currentDEM)
    {
        m_demPipelineStart->SetInputDataObject(currentDEM->dataSet());
        assert(currentDEM->dataSet()->GetPointData()->GetScalars());

        const auto newDEMBounds = DataBounds(currentDEM->bounds());
        if (newDEMBounds == m_previousDEMBounds)
        {
            needToResetCamera = false;
        }
        else
        {
            m_previousDEMBounds = newDEMBounds;
        }
    }
    else
    {
        auto nullDEM = vtkSmartPointer<vtkImageData>::New();
        nullDEM->SetExtent(0, 0, 0, 0, 0, 0);
        nullDEM->AllocateScalars(VTK_FLOAT, 1);
        reinterpret_cast<float *>(nullDEM->GetScalarPointer())[0] = 0.f;
        nullDEM->GetPointData()->GetScalars()->SetName("DEMdata");
        m_demPipelineStart->SetInputDataObject(nullDEM);

        m_previousDEMBounds = DataBounds();
    }
    
    m_meshPipelineStart->SetInputDataObject(currentTopo.dataSet());


    // setup default parameters
    if (currentDEM)
    {
        matchTopoMeshRadius();
        centerTopoMesh();
    }

    
    m_pipelineEnd->Update();

    vtkPolyData * newDataSet = vtkPolyData::SafeDownCast(m_pipelineEnd->GetOutputDataObject(0));

    if (!newDataSet)
    {
        qDebug() << "DEMWidget: mesh transformation did not succeed";
        return;
    }

    m_dataPreview = std::make_unique<PolyDataObject>("", *newDataSet);
    m_renderedPreview = m_dataPreview->createRendered();

    auto props = m_renderedPreview->viewProps();
    props->InitTraversal();
    while (auto p = props->GetNextProp())
    {
        vtkActor * actor = nullptr; vtkProperty * property = nullptr;
        if ((actor = vtkActor::SafeDownCast(p)) && (property = actor->GetProperty()))
        {
            property->LightingOn();
            property->EdgeVisibilityOff();
        }

        m_renderer->AddViewProp(p);
    }
    m_renderer->AddViewProp(m_axesActor);

    if (needToResetCamera)
    {
        m_renderer->ResetCamera();
    }

    updateView();

    m_ui->newTopoModelName->setText(currentTopo.name() + (currentDEM ? " (" + currentDEM->name() + ")" : ""));
}

void DEMWidget::setupPipeline()
{
    m_meshTransform = vtkSmartPointer<vtkTransform>::New();
    auto meshTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    meshTransformFilter->SetTransform(m_meshTransform);
    m_meshPipelineStart = meshTransformFilter;

    updateMeshTransform();

    m_demUnitScale = vtkSmartPointer<vtkImageShiftScale>::New();
    m_demPipelineStart = m_demUnitScale;

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(meshTransformFilter->GetOutputPort());
    probe->SetSourceConnection(m_demUnitScale->GetOutputPort());

    auto warpElevation = vtkSmartPointer<vtkWarpScalar>::New();
    warpElevation->SetInputConnection(probe->GetOutputPort());
    m_pipelineEnd = warpElevation;
}

void DEMWidget::updateAvailableDataSets()
{
    m_topographyMeshes.clear();
    m_dems.clear();
    m_ui->topoTemplateCombo->clear();
    m_ui->demCombo->clear();

    for (auto dataObject : m_dataSetHandler.dataSets())
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
}

void DEMWidget::updateMeshTransform()
{
    m_meshTransform->Identity();
    m_meshTransform->PostMultiply();

    m_meshTransform->Scale(m_topoRadiusScale, m_topoRadiusScale, 0.0);
    m_meshTransform->Translate(m_topoShift.GetData()); // assuming the template is already centered around (0,0,z)
}

void DEMWidget::updateView()
{
    m_pipelineEnd->Update();

    if (m_dataPreview && m_axesActor)
    {
        double bounds[6];
        m_dataPreview->bounds(bounds);
        m_axesActor->SetGridBounds(bounds);
    }

    m_ui->qvtkMain->GetRenderWindow()->Render();
}

void DEMWidget::matchTopoMeshRadius()
{
    auto currentDEM = m_dems.value(m_ui->demCombo->currentIndex(), nullptr);
    auto currentTopo = m_topographyMeshes.value(m_ui->topoTemplateCombo->currentIndex(), nullptr);

    if (!currentDEM || !currentTopo)
    {
        return;
    }

    const auto demSize = DataBounds(currentDEM->bounds()).size();
    const auto topoSize = DataBounds(currentTopo->bounds()).size();

    const auto topoScalePerAxis = demSize / topoSize;

    // scale the topography so that it fits into the DEM
    const double newScale = std::min(topoScalePerAxis[0], topoScalePerAxis[1]);
    if (newScale == m_topoRadiusScale)
    {
        return;
    }
    m_topoRadiusScale = newScale;

    QSignalBlocker signalBlocker(m_ui->topographyRadiusSpinBox);

    m_ui->topographyRadiusSpinBox->setMaximum(m_topoRadiusScale);   // don't allow the mesh to be stretched beyond the DEM
    m_ui->topographyRadiusSpinBox->setValue(m_topoRadiusScale);

    updateMeshTransform();
}

void DEMWidget::centerTopoMesh()
{
    auto currentDEM = m_dems.value(m_ui->demCombo->currentIndex(), nullptr);

    if (!currentDEM)
    {
        return;
    }

    const auto signalBlockers = {
        QSignalBlocker(m_ui->topographyCenterXSpinBox),
        QSignalBlocker(m_ui->topographyCenterYSpinBox) };

    const auto demCenter = DataBounds(currentDEM->bounds()).center();
    const auto newShift = vtkVector3d{ demCenter[0], demCenter[1], 0.0 };
    if (newShift == m_topoShift)
    {
        return;
    }

    m_topoShift = newShift;

    m_ui->topographyCenterXSpinBox->setValue(m_topoShift[0]);
    m_ui->topographyCenterYSpinBox->setValue(m_topoShift[1]);

    updateMeshTransform();
}
