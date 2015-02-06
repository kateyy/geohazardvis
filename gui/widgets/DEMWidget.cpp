#include <gui/widgets/DEMWidget.h>
#include "ui_DEMWidget.h"

#include <QMessageBox>

#include <vtkCamera.h>
#include <vtkPropCollection.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>

#include <core/DataSetHandler.h>
#include <core/vtkcamerahelper.h>
#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>


DEMWidget::DEMWidget(QWidget * parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_ui{ new Ui_DEMWidget }
    , m_dataPreview{ nullptr }
    , m_renderedPreview{ nullptr }
{
    m_ui->setupUi(this);

    for (DataObject * data : DataSetHandler::instance().dataSets())
    {
        if (auto p = dynamic_cast<PolyDataObject *>(data))
        {
            if (p->is2p5D())
                m_surfacesMeshes << p;
        }
        else if (auto i = dynamic_cast<ImageDataObject *>(data))
            m_dems << i;
    }

    for (auto p : m_surfacesMeshes)
        m_ui->surfaceMeshCombo->addItem(p->name());

    for (auto d : m_dems)
        m_ui->demCombo->addItem(d->name());

    connect(m_ui->surfaceMeshCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DEMWidget::updatePreview);
    connect(m_ui->demCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DEMWidget::updatePreview);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &DEMWidget::saveAndClose);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);
}

DEMWidget::~DEMWidget()
{
    m_renderer->RemoveAllViewProps();
    delete m_renderedPreview;
    delete m_dataPreview;
}

bool DEMWidget::save()
{
    if (!m_dataPreview)
    {
        QMessageBox::information(this, "", "Select a surface mesh and a DEM to apply before!");
        return false;
    }

    auto newData = new PolyDataObject(m_ui->newSurfaceModelName->text(), m_dataPreview->polyDataSet());
    DataSetHandler::instance().addData({ newData });

    return true;
}

void DEMWidget::saveAndClose()
{
    if (save())
        close();
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

    updatePreview();
}

void DEMWidget::updatePreview()
{
    m_renderer->RemoveAllViewProps();
    delete m_renderedPreview;
    delete m_dataPreview;
    m_dataPreview = nullptr;
    m_renderedPreview = nullptr;

    int surfaceIdx = m_ui->surfaceMeshCombo->currentIndex();
    if (surfaceIdx == -1)
        return;

    int demIndex = m_ui->demCombo->currentIndex();
    ImageDataObject * dem = m_dems.value(demIndex, nullptr);
    PolyDataObject * surface = m_surfacesMeshes[surfaceIdx];


    setupDEMStages();
    m_demTransform->SetInputConnection(dem->processedOutputPort());
    m_meshTransform->SetInputConnection(surface->processedOutputPort());

    m_demWarpElevation->Update();
    vtkPolyData * newDataSet = vtkPolyData::SafeDownCast(m_demWarpElevation->GetOutput());
    assert(newDataSet);

    m_dataPreview = new PolyDataObject("", newDataSet);
    m_renderedPreview = m_dataPreview->createRendered();

    auto props = m_renderedPreview->viewProps();
    props->InitTraversal();
    while (auto p = props->GetNextProp())
        m_renderer->AddViewProp(p);

    m_renderer->ResetCamera();


    m_ui->newSurfaceModelName->setText(surface->name() + (dem ? " (" + dem->name() + ")" : ""));
}

void DEMWidget::setupDEMStages()
{
    if (m_demWarpElevation)
        return;

    const double earthR = 6378.138;
    double Fi0 = -25.167916666667;  // latitude 
    double La0 = -68.5062500000003; // longitude of the origin (~center) of the surface's local coordinate system


    // approximations for regions not larger than a few hundreds of kilometers:
    /*auto transformApprox = [earthR] (double Fi, double La, double Fi0, double La0, double & X, double & Y)
    {
    Y = earthR * (Fi - Fi0) * vtkMath::Pi() / 180;
    X = earthR * (La - La0) * std::cos(Fi0 / 180 * vtkMath::Pi()) * vtkMath::Pi() / 180;
    };*/

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

    m_demTransform = vtkSmartPointer<vtkImageShiftScale>::New();
    m_demTransform->SetScale(0.001);

    VTK_CREATE(vtkImageChangeInformation, demTranslation);
    demTranslation->SetInputConnection(m_demTransform->GetOutputPort());
    demTranslation->SetOriginTranslation(toLocalTranslation);

    VTK_CREATE(vtkImageChangeInformation, demScale);
    demScale->SetInputConnection(demTranslation->GetOutputPort());
    demScale->SetSpacingScale(toLocalScale);
    demScale->SetOriginScale(toLocalScale);

    VTK_CREATE(vtkTransform, meshTransform);
    meshTransform->Scale(1, 1, 0);  // flatten

    m_meshTransform = vtkSmartPointer<vtkTransformFilter>::New();
    m_meshTransform->SetTransform(meshTransform);

    VTK_CREATE(vtkProbeFilter, probe);
    probe->SetInputConnection(m_meshTransform->GetOutputPort());
    probe->SetSourceConnection(demScale->GetOutputPort());
    probe->PassCellArraysOn();
    probe->PassPointArraysOn();

    m_demWarpElevation = vtkSmartPointer<vtkWarpScalar>::New();
    m_demWarpElevation->SetInputConnection(probe->GetOutputPort());
}
