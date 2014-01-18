#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkCamera.h>

// inputs
#include <vtkPolyDataAlgorithm.h>
// filters
#include <vtkElevationFilter.h>
// mappers
#include <vtkPolyDataMapper.h>
// actors
#include <vtkActor.h>
#include <vtkAxisActor.h>
#include <vtkCubeAxesActor.h>
// rendering
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
// interaction
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include "pickinginteractionstyle.h"
#include <vtkBoxWidget.h>
#include <vtkPointPicker.h>
// gui/qt
#include <QVTKWidget.h>
#include <QTreeView>

#include "core/loader.h"

class TransformCallback : public vtkCommand
{
public:
    static TransformCallback *New()
    {
        return new TransformCallback;
    }
    virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
        vtkTransform *t = vtkTransform::New();
        vtkBoxWidget *widget = dynamic_cast<vtkBoxWidget*>(caller);
        assert(widget);
        widget->GetTransform(t);
        widget->GetProp3D()->SetUserTransform(t);
        t->Delete();
    }
};

Viewer::Viewer()
: m_ui(new Ui_Viewer())
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();

    loadInputs();

    m_mainRenderer->ResetCamera();
}

void Viewer::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(4);

    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    m_infoRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_infoRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkInfo->GetRenderWindow()->AddRenderer(m_infoRenderer);

    vtkCamera & camera = *m_mainRenderer->GetActiveCamera();

    camera.SetPosition(0.3, -1, 0.3);
    camera.SetViewUp(0, 0, 1);

    m_mainRenderer->ResetCamera();
}

void Viewer::setupInteraction()
{
    vtkSmartPointer<PickingInteractionStyle> interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    interactStyle->SetDefaultRenderer(m_mainRenderer);
    interactStyle->setViewer(*this);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
}

void Viewer::loadInputs()
{
    m_inputs.push_back(Loader::loadFileTriangulated("data/Tcoord_topo.txt", 4, 1));
    m_inputs.push_back(Loader::loadFileAsPoints("data/Tvert.txt", 6, 0));

    std::shared_ptr<Input> input1 = m_inputs.front();
    std::shared_ptr<Input> input2 = m_inputs.back();

    // use the elevation for colorized visualization
    vtkSmartPointer<vtkElevationFilter> elevation = vtkSmartPointer<vtkElevationFilter>::New();
    elevation->SetInputConnection(input1->algorithm->GetOutputPort());
    elevation->SetLowPoint(0, 0, 4);
    elevation->SetHighPoint(0, 0, 0);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(elevation->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    vtkProperty & prop = *actor->GetProperty();
    prop.SetOpacity(0.6);
    prop.SetInterpolationToGouraud();
    prop.SetEdgeVisibility(true);
    prop.SetEdgeColor(0, 0, 0);
    prop.SetBackfaceCulling(false);
    prop.SetLighting(false);

    m_mainRenderer->AddActor(actor);
    vtkSmartPointer<vtkActor> actor2 = input2->createActor();
    actor2->GetProperty()->SetColor(0, 1, 0);
    m_infoRenderer->AddActor(actor2);

    setupAxis(*input1);
}

void Viewer::setupAxis(const Input & input)
{
    vtkSmartPointer<vtkCubeAxesActor> cubeAxes = vtkSmartPointer<vtkCubeAxesActor>::New();
    cubeAxes->SetBounds(input.polyData->GetBounds());
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

    m_mainRenderer->AddActor(cubeAxes);
}

void Viewer::ShowInfo(const QStringList & info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItems(info);
}
