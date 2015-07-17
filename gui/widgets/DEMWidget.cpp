#include <gui/widgets/DEMWidget.h>
#include "ui_DEMWidget.h"

#include <QMessageBox>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCubeAxesActor.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextProperty.h>

#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDoubleArray.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>

#include <core/DataSetHandler.h>
#include <core/utility/vtkcamerahelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>


DEMWidget::DEMWidget(QWidget * parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui{ new Ui_DEMWidget }
    , m_currentDEM{ nullptr }
    , m_dataPreview{ nullptr }
    , m_renderedPreview{ nullptr }
{
    m_ui->setupUi(this);

    updateAvailableDataSets();

    setupDEMStages();

    connect(m_ui->surfaceMeshCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DEMWidget::updatePreview);
    connect(m_ui->demCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DEMWidget::updatePreview);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &DEMWidget::saveAndClose);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

    connect(m_ui->surfaceScaleX, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this] (double) {
        updateMeshScale();
        updateView();
    });
    connect(m_ui->surfaceScaleY, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this] (double) {
        updateMeshScale();
        updateView();
    });

    connect(m_ui->demLatitude, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this] (double) {
        updateDEMGeoPosition();
        updateView();
    });
    connect(m_ui->demLongitude, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this] (double) {
        updateDEMGeoPosition();
        updateView();
    });


    connect(&DataSetHandler::instance(), &DataSetHandler::dataObjectsChanged, this, &DEMWidget::updateAvailableDataSets);
}

DEMWidget::~DEMWidget()
{
    m_renderer->RemoveAllViewProps();
}

bool DEMWidget::save()
{
    if (!m_dataPreview)
    {
        QMessageBox::information(this, "", "Select a surface mesh and a DEM to apply before!");
        return false;
    }

    auto surface = vtkSmartPointer<vtkPolyData>::New();
    surface->DeepCopy(m_dataPreview->polyDataSet());

    // remove arrays that were created while appying the DEM
    surface->GetPointData()->RemoveArray("vtkValidPointMask");
    surface->GetPointData()->RemoveArray(m_demScalarsName.toUtf8().data());

    // store size and position of the DEM in the surface's field data

    auto addArray = [surface] (const char * name, const std::vector<double> & values) {
        auto a = vtkSmartPointer<vtkDoubleArray>::New();
        a->SetName(name);
        a->SetNumberOfValues(vtkIdType(values.size()));
        for (unsigned i = 0; i < values.size(); ++i)
            a->SetValue(i, values[i]);
        surface->GetFieldData()->AddArray(a);
    };

    if (m_currentDEM)
    {
        m_demTransformOutput->Update();
        vtkDataSet * transformedDEM = vtkDataSet::SafeDownCast(m_demTransformOutput->GetOutputDataObject(0));
        assert(transformedDEM);
        std::vector<double> demBounds(6u);
        transformedDEM->GetBounds(demBounds.data());

        assert(surface->GetFieldData()->GetNumberOfArrays() == 0);

        auto demNameArray = vtkSmartPointer<vtkCharArray>::New();
        demNameArray->SetName("DEM_Name");
        QByteArray demName = m_currentDEM->name().toUtf8();
        demNameArray->SetNumberOfValues(demName.size());
        for (int i = 0; i < demName.size(); ++i)
            demNameArray->SetValue(i, demName[i]);
        surface->GetFieldData()->AddArray(demNameArray);

        addArray("DEM_F0", { m_ui->demLatitude->value() });
        addArray("DEM_La0", { m_ui->demLongitude->value() });
        addArray("DEM_Bounds", demBounds);
    }

    auto newData = std::make_unique<PolyDataObject>(m_ui->newSurfaceModelName->text(), *surface);
    DataSetHandler::instance().takeData(std::move(newData));

    return true;
}

void DEMWidget::saveAndClose()
{
    disconnect(&DataSetHandler::instance(), &DataSetHandler::dataObjectsChanged,
        this, &DEMWidget::updateAvailableDataSets);

    if (save())
        close();

    updateAvailableDataSets();

    connect(&DataSetHandler::instance(), &DataSetHandler::dataObjectsChanged,
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

    vtkSmartPointer<InteractorStyle3D> interactorStyle = vtkSmartPointer<InteractorStyle3D>::New();
    m_ui->qvtkMain->GetInteractor()->SetInteractorStyle(interactorStyle);
    interactorStyle->SetDefaultRenderer(m_renderer);


    m_axesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    m_axesActor->SetCamera(m_renderer->GetActiveCamera());
    m_axesActor->SetFlyModeToOuterEdges();
    m_axesActor->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);
    //m_axesActor->SetUseTextActor3D(true);
    m_axesActor->SetTickLocationToBoth();
    // fix strange rotation of z-labels
    m_axesActor->GetLabelTextProperty(2)->SetOrientation(90);

    double axesColor[3] = { 0, 0, 0 };
    double gridColor[3] = { 0.7, 0.7, 0.7 };

    m_axesActor->GetXAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetYAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetZAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetXAxesGridlinesProperty()->SetColor(gridColor);
    m_axesActor->GetYAxesGridlinesProperty()->SetColor(gridColor);
    m_axesActor->GetZAxesGridlinesProperty()->SetColor(gridColor);

    for (int i = 0; i < 3; ++i)
    {
        m_axesActor->GetTitleTextProperty(i)->SetColor(axesColor);
        m_axesActor->GetLabelTextProperty(i)->SetColor(axesColor);
    }

    m_axesActor->XAxisMinorTickVisibilityOff();
    m_axesActor->YAxisMinorTickVisibilityOff();
    m_axesActor->ZAxisMinorTickVisibilityOff();

    m_axesActor->DrawXGridlinesOn();
    m_axesActor->DrawYGridlinesOn();
    m_axesActor->DrawZGridlinesOn();

    updatePreview();
}

void DEMWidget::updatePreview()
{
    m_renderer->RemoveAllViewProps();
    m_renderedPreview.reset();
    m_dataPreview.reset();

    int surfaceIdx = m_ui->surfaceMeshCombo->currentIndex();
    if (surfaceIdx == -1)
        return;

    int demIndex = m_ui->demCombo->currentIndex();
    m_currentDEM = m_dems.value(demIndex, nullptr);
    PolyDataObject * surface = m_surfaceMeshes[surfaceIdx];

    if (m_currentDEM)
    {
        m_demTranslate->SetInputDataObject(m_currentDEM->dataSet());
        m_demScalarsName = QString::fromUtf8(
            m_currentDEM->dataSet()->GetPointData()->GetScalars()->GetName());
    }
    else
    {
        auto nullDEM = vtkSmartPointer<vtkImageData>::New();
        nullDEM->SetExtent(0, 0, 0, 0, 0, 0);
        nullDEM->AllocateScalars(VTK_FLOAT, 1);
        reinterpret_cast<float *>(nullDEM->GetScalarPointer())[0] = 0.f;
        m_demScalarsName = "DEMdata";
        nullDEM->GetPointData()->GetScalars()->SetName(
            m_demScalarsName.toUtf8().data());
        m_demTranslate->SetInputData(nullDEM);
    }
    


    m_meshTransform->SetInputData(surface->dataSet());
    
    m_demWarpElevation->Update();

    vtkPolyData * newDataSet = vtkPolyData::SafeDownCast(m_demWarpElevation->GetOutput());
    assert(newDataSet);

    m_dataPreview = std::make_unique<PolyDataObject>("", *newDataSet);
    m_renderedPreview = m_dataPreview->createRendered();

    auto props = m_renderedPreview->viewProps();
    props->InitTraversal();
    while (auto p = props->GetNextProp())
    {
        vtkActor * actor; vtkProperty * property;
        if ((actor = vtkActor::SafeDownCast(p)) && (property = actor->GetProperty()))
        {
            property->LightingOn();
            property->EdgeVisibilityOff();
        }

        m_renderer->AddViewProp(p);
    }
    m_renderer->AddViewProp(m_axesActor);

    updateView();

    m_renderer->ResetCamera();

    m_ui->newSurfaceModelName->setText(surface->name() + (m_currentDEM ? " (" + m_currentDEM->name() + ")" : ""));
}

void DEMWidget::setupDEMStages()
{
    m_demTranslate = vtkSmartPointer<vtkImageChangeInformation>::New();

    m_demScale = vtkSmartPointer<vtkImageChangeInformation>::New();
    m_demScale->SetInputConnection(m_demTranslate->GetOutputPort());

    auto scaleToKm = vtkSmartPointer<vtkImageShiftScale>::New();
    scaleToKm->SetInputConnection(m_demScale->GetOutputPort());
    scaleToKm->SetScale(0.001);
    m_demTransformOutput = scaleToKm;

    updateDEMGeoPosition();

    auto meshTransform = vtkSmartPointer<vtkTransform>::New();

    m_meshTransform = vtkSmartPointer<vtkTransformFilter>::New();
    m_meshTransform->SetTransform(meshTransform);

    updateMeshScale();

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(m_meshTransform->GetOutputPort());
    probe->SetSourceConnection(scaleToKm->GetOutputPort());
    probe->PassCellArraysOn();
    probe->PassPointArraysOn();

    m_demWarpElevation = vtkSmartPointer<vtkWarpScalar>::New();
    m_demWarpElevation->SetInputConnection(probe->GetOutputPort());
}

void DEMWidget::updateAvailableDataSets()
{
    m_surfaceMeshes.clear();
    m_dems.clear();
    m_ui->surfaceMeshCombo->clear();
    m_ui->demCombo->clear();

    for (DataObject * data : DataSetHandler::instance().dataSets())
    {
        if (auto p = dynamic_cast<PolyDataObject *>(data))
        {
            if (p->is2p5D())
                m_surfaceMeshes << p;
        }
        else if (auto i = dynamic_cast<ImageDataObject *>(data))
            m_dems << i;
    }

    for (auto p : m_surfaceMeshes)
        m_ui->surfaceMeshCombo->addItem(p->name());

    for (auto d : m_dems)
        m_ui->demCombo->addItem(d->name());
}

void DEMWidget::updateDEMGeoPosition()
{
    const double earthR = 6378.138;

    // approximations for regions not larger than a few hundreds of kilometers:
    /*auto transformApprox = [earthR] (double Fi, double La, double Fi0, double La0, double & X, double & Y)
    {
    Y = earthR * (Fi - Fi0) * vtkMath::Pi() / 180;
    X = earthR * (La - La0) * std::cos(Fi0 / 180 * vtkMath::Pi()) * vtkMath::Pi() / 180;
    };*/

    double Fi0 = m_ui->demLatitude->value();
    double La0 = m_ui->demLongitude->value();

    double toLocalTranslation[3] = {
        -La0,
        -Fi0,
        0.0
    };
    double toLocalScale[3] = {
        earthR * std::cos(Fi0 / 180.0 * vtkMath::Pi()) * vtkMath::Pi() / 180.0,
        earthR * vtkMath::Pi() / 180.0,
        0  // flattening, elevation is stored in scalars
    };

    m_demTranslate->SetOriginTranslation(toLocalTranslation);

    m_demScale->SetSpacingScale(toLocalScale);
    m_demScale->SetOriginScale(toLocalScale);

}

void DEMWidget::updateMeshScale()
{
    auto tr = vtkTransform::SafeDownCast(m_meshTransform->GetTransform());
    tr->Identity();
    tr->Scale(
        m_ui->surfaceScaleX->value(),
        m_ui->surfaceScaleY->value(),
        0); // flatten

}

void DEMWidget::updateView()
{
    m_demWarpElevation->Update();

    if (m_dataPreview && m_axesActor)
    {
        double bounds[6];
        m_dataPreview->bounds(bounds);
        m_axesActor->SetBounds(bounds);
    }

    m_ui->qvtkMain->GetRenderWindow()->Render();
}
