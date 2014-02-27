#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkCamera.h>
#include <vtkLookupTable.h>

// inputs
#include <vtkPolyDataAlgorithm.h>
// filters
#include <vtkElevationFilter.h>
// mappers
#include <vtkPolyDataMapper.h>
// actors
#include <vtkActor.h>
#include <vtkCubeAxesActor.h>
#include <vtkScalarBarActor.h>
// rendering
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
// interaction
#include "pickinginteractionstyle.h"
// gui/qt
#include <QVTKWidget.h>
#include <QFileDialog>
#include <QMessageBox>

#include "core/loader.h"
#include "core/input.h"

using namespace std;

Viewer::Viewer()
: m_ui(new Ui_Viewer())
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();

    m_mainRenderer->ResetCamera();
    m_infoRenderer->ResetCamera();
}

void Viewer::setupRenderer()
{
    //m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(2);

    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    m_infoRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_infoRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkInfo->GetRenderWindow()->AddRenderer(m_infoRenderer);
}

void Viewer::setupInteraction()
{
    vtkSmartPointer<PickingInteractionStyle> interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    interactStyle->SetDefaultRenderer(m_mainRenderer);
    connect(&interactStyle->pickingInfo, SIGNAL(infoSent(const QStringList&)), SLOT(ShowInfo(const QStringList&)));
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();


    vtkSmartPointer<PickingInteractionStyle> interactStyleInfo = vtkSmartPointer<PickingInteractionStyle>::New();
    interactStyleInfo->SetDefaultRenderer(m_infoRenderer);

    connect(&interactStyleInfo->pickingInfo, SIGNAL(infoSent(const QStringList&)), SLOT(ShowInfo(const QStringList&)));
    m_infoInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_infoInteractor->SetInteractorStyle(interactStyleInfo);
    m_infoInteractor->SetRenderWindow(m_ui->qvtkInfo->GetRenderWindow());

    m_infoInteractor->Initialize();
}

void Viewer::ShowInfo(const QStringList & info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItems(info);

    setToolTip(info.join('\n'));
}

void Viewer::on_actionOpen_triggered()
{
    static QString lastFolder;
    QString filename = QFileDialog::getOpenFileName(this, "", lastFolder, "Text files (*.txt)");
    if (filename.isEmpty())
        return;

    lastFolder = QFileInfo(filename).absolutePath();

    QByteArray fndata = filename.toLatin1().data(); // work around for qt libraries used with VS11, but compiled for VS10..
    string fnStr(fndata.data());
    shared_ptr<Input> input = Loader::readFile(fnStr);
    if (!input) {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        return;
    }

    m_inputs = {input};

    m_mainRenderer->RemoveAllViewProps();

    switch (input->type) {
    case ModelType::triangles:
        show3DInput(*static_cast<Input3D*>(input.get()));
        break;
    case ModelType::grid2d:
        showGridInput(*static_cast<GridDataInput*>(input.get()));
        break;
    default:
        QMessageBox::critical(this, "File error", "Could not open the selected input file. (unsupported format)");
    }

    vtkCamera & camera = *m_mainRenderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_mainRenderer->ResetCamera();
    m_ui->qvtkMain->GetRenderWindow()->Render();
}

void Viewer::show3DInput(Input3D & input)
{
    vtkSmartPointer<vtkActor> actor = input.createActor();
    vtkProperty & prop = *actor->GetProperty();
    prop.SetColor(1, 1, 0);
    prop.SetOpacity(1.0);
    prop.SetInterpolationToGouraud();
    prop.SetEdgeVisibility(true);
    prop.SetEdgeColor(0.1, 0.1, 0.1);
    prop.SetLineWidth(1.5);
    prop.SetBackfaceCulling(false);
    prop.SetLighting(true);

    m_mainRenderer->AddViewProp(actor);

    setupAxes(input.data()->GetBounds());
}

void Viewer::showGridInput(GridDataInput & input)
{
    vtkScalarBarActor * heatBars = vtkScalarBarActor::New();
    heatBars->SetTitle(input.name.c_str());
    heatBars->SetLookupTable(input.lookupTable);
    m_mainRenderer->AddViewProp(heatBars);
    m_mainRenderer->AddViewProp(input.createTexturedPolygonActor());

    setupAxes(input.bounds);
}

void Viewer::setupAxes(const double bounds[6])
{
    if (!m_axesActor) {
        m_axesActor = createAxes(*m_mainRenderer);
        m_mainRenderer->AddViewProp(m_axesActor);
    }
    double b[6];
    for (int i = 0; i < 6; ++i)
        b[i] = bounds[i];
    m_axesActor->SetBounds(b);
    m_axesActor->SetRebuildAxes(true);
}

vtkCubeAxesActor * Viewer::createAxes(vtkRenderer & renderer)
{
    vtkCubeAxesActor * cubeAxes = vtkCubeAxesActor::New();
    cubeAxes->SetCamera(m_mainRenderer->GetActiveCamera());
    cubeAxes->SetFlyModeToOuterEdges();
    cubeAxes->SetEnableDistanceLOD(1);
    cubeAxes->SetEnableViewAngleLOD(1);
    cubeAxes->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);

    double axesColor[3] = {0, 0, 0};
    double gridColor[3] = {0.7, 0.7, 0.7};

    cubeAxes->GetXAxesLinesProperty()->SetColor(axesColor);
    cubeAxes->GetYAxesLinesProperty()->SetColor(axesColor);
    cubeAxes->GetZAxesLinesProperty()->SetColor(axesColor);
    cubeAxes->GetXAxesGridlinesProperty()->SetColor(gridColor);
    cubeAxes->GetYAxesGridlinesProperty()->SetColor(gridColor);
    cubeAxes->GetZAxesGridlinesProperty()->SetColor(gridColor);

    for (int i = 0; i < 3; ++i) {
        cubeAxes->GetTitleTextProperty(i)->SetColor(axesColor);
        cubeAxes->GetLabelTextProperty(i)->SetColor(axesColor);
    }

    cubeAxes->SetXAxisMinorTickVisibility(0);
    cubeAxes->SetYAxisMinorTickVisibility(0);
    cubeAxes->SetZAxisMinorTickVisibility(0);

    cubeAxes->DrawXGridlinesOn();
    cubeAxes->DrawYGridlinesOn();
    cubeAxes->DrawZGridlinesOn();

    cubeAxes->SetRebuildAxes(true);

    return cubeAxes;
}
