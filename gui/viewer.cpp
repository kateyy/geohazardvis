#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
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
#include "core/input.h"

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


    vtkSmartPointer<PickingInteractionStyle> interactStyleInfo = vtkSmartPointer<PickingInteractionStyle>::New();
    interactStyleInfo->SetDefaultRenderer(m_infoRenderer);
    interactStyleInfo->setViewer(*this);
    m_infoInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_infoInteractor->SetInteractorStyle(interactStyleInfo);
    m_infoInteractor->SetRenderWindow(m_ui->qvtkInfo->GetRenderWindow());

    m_infoInteractor->Initialize();
}

void Viewer::loadInputs()
{
    {
        std::shared_ptr<Input> indexedSphere = Loader::loadIndexedTriangles(
            "data/Spcoord.txt", 0, 1,
            "data/Svert.txt", 0);
        m_inputs.push_back(indexedSphere);

        vtkSmartPointer<vtkActor> sphereActor = indexedSphere->createActor();
        vtkProperty & prop = *sphereActor->GetProperty();
        prop.SetColor(1, 1, 0);
        prop.SetOpacity(0.9);
        prop.SetInterpolationToGouraud();
        prop.SetEdgeVisibility(true);
        prop.SetEdgeColor(0, 0, 0);
        prop.SetLineWidth(1.5);
        prop.SetBackfaceCulling(false);
        prop.SetLighting(true);

        vtkSmartPointer<vtkActor> sphereActorInfo = indexedSphere->createActor();
        sphereActorInfo->ShallowCopy(sphereActor);

        //m_mainRenderer->AddActor(sphereActor);
        m_infoRenderer->AddActor(sphereActor);
    }

    {
        //std::shared_ptr<ProcessedInput> processedVolcano = Loader::loadFileTriangulated("data/Tcoord_topo.txt", 1);
        //m_inputs.push_back(processedVolcano);

        //// use the elevation for colorized visualization
        //vtkSmartPointer<vtkElevationFilter> elevation = vtkSmartPointer<vtkElevationFilter>::New();
        //elevation->SetInputConnection(processedVolcano->algorithm->GetOutputPort());
        //elevation->SetLowPoint(0, 0, 4);
        //elevation->SetHighPoint(0, 0, 0);

        //vtkSmartPointer<vtkPolyDataMapper> mapper = processedVolcano->createAlgorithmMapper(
        //    elevation->GetOutputPort());

        //vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        //actor->SetMapper(mapper);
        //vtkProperty & prop = *actor->GetProperty();
        //prop.SetOpacity(0.6);
        //prop.SetInterpolationToFlat();
        //prop.SetEdgeVisibility(true);
        //prop.SetEdgeColor(0, 0, 0);
        //prop.SetBackfaceCulling(false);
        //prop.SetLighting(false);

        //m_mainRenderer->AddActor(actor);
        //setupAxis(*processedVolcano, *m_mainRenderer);
    }

    {
        std::shared_ptr<DataSetInput> heatMap = Loader::loadGrid("data/observation.txt", "data/X.txt", "data/Y.txt");
        m_inputs.push_back(heatMap);

        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->SetHueRange(0.66667, 0.0);
        lut->Build();

        heatMap->mapper()->SetScalarRange(heatMap->minMaxValue());
        heatMap->mapper()->SetLookupTable(lut);

        m_mainRenderer->AddActor(heatMap->createActor());
    }
}

void Viewer::setupAxis(const Input & input, vtkRenderer & renderer)
{
    vtkSmartPointer<vtkCubeAxesActor> cubeAxes = vtkSmartPointer<vtkCubeAxesActor>::New();
    cubeAxes->SetBounds(input.data()->GetBounds());
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

    renderer.AddActor(cubeAxes);
}

void Viewer::ShowInfo(const QStringList & info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItems(info);

    setToolTip(info.join('\n'));
}
