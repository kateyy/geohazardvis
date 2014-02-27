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

#include "core/loader.h"
#include "core/input.h"

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

    //vtkCamera & camera = *m_mainRenderer->GetActiveCamera();
    //camera.SetPosition(0.3, -1, 0.3);
    //camera.SetViewUp(0, 0, 1);
    //m_mainRenderer->ResetCamera();
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

void Viewer::loadInputs()
{
    {
        /*std::shared_ptr<ProcessedInput> processedVolcano = Loader::loadFileTriangulated("data/Tcoord_topo.txt", 1);
        m_inputs.push_back(processedVolcano);*/

        std::shared_ptr<Input3D> volcano = Loader::loadIndexedTriangles("data/volcano.txt");
        m_inputs.push_back(volcano);


        // use the elevation for colorized visualization
 /*       vtkSmartPointer<vtkElevationFilter> elevation = vtkSmartPointer<vtkElevationFilter>::New();
        elevation->SetInputConnection(processedVolcano->algorithm->GetOutputPort());
        elevation->SetLowPoint(0, 0, 4);
        elevation->SetHighPoint(0, 0, 0);*/

        //vtkSmartPointer<vtkPolyDataMapper> mapper = processedVolcano->createAlgorithmMapper(
        //    elevation->GetOutputPort());

        /*vtkSmartPointer<vtkActor> volcanoActor = vtkSmartPointer<vtkActor>::New();
        volcanoActor->SetMapper(mapper);*/
        //vtkProperty & prop = *volcanoActor->GetProperty();

        vtkSmartPointer<vtkActor> volcanoActor = volcano->createActor();
        vtkProperty & prop = *volcanoActor->GetProperty();
        prop.SetOpacity(0.6);
        prop.SetInterpolationToFlat();
        prop.SetEdgeVisibility(true);
        prop.SetEdgeColor(0, 0, 0);
        prop.SetBackfaceCulling(false);
        prop.SetLighting(false);

        m_loadedInputs.insert("volcano", QVector<vtkSmartPointer<vtkProp>>({
            volcanoActor,
            createAxes(volcano->data()->GetBounds(), *m_mainRenderer)}));
    }

    {
        std::shared_ptr<Input3D> indexedSphere = Loader::loadIndexedTriangles("data/sphere.txt");
        m_inputs.push_back(indexedSphere);

  /*      std::shared_ptr<Input3D> indexedSphere = Loader::loadIndexedTriangles(
            "data/Spcoord.txt", 0, 1,
            "data/Svert.txt", 0);
        m_inputs.push_back(indexedSphere);*/

        vtkSmartPointer<vtkActor> sphereActor = indexedSphere->createActor();
        vtkProperty & prop = *sphereActor->GetProperty();
        prop.SetColor(1, 1, 0);
        prop.SetOpacity(1.0);
        prop.SetInterpolationToGouraud();
        prop.SetEdgeVisibility(true);
        prop.SetEdgeColor(0, 0, 0);
        prop.SetLineWidth(1.5);
        prop.SetBackfaceCulling(false);
        prop.SetLighting(true);

        m_infoRenderer->AddViewProp(sphereActor);

        m_loadedInputs.insert("sphere", QVector<vtkSmartPointer<vtkProp>>({ sphereActor }));
        m_loadedInputs["volcano"].push_back(sphereActor);
    }

    {
        //std::shared_ptr<GridDataInput> heatMap = Loader::loadGrid("data/observation.txt", "data/X.txt", "data/Y.txt");
        std::shared_ptr<GridDataInput> heatMap = Loader::loadGrid("data/displacements.txt");
        m_inputs.push_back(heatMap);

        vtkScalarBarActor * heatBars = vtkScalarBarActor::New();
        heatBars->SetTitle(heatMap->name.c_str());
        heatBars->SetLookupTable(heatMap->lookupTable);

        m_loadedInputs.insert("displacements", QVector<vtkSmartPointer<vtkProp>>({
            heatMap->createTexturedPolygonActor(),
            heatBars,
            createAxes(heatMap->bounds, *m_mainRenderer) }));
    }
}

vtkCubeAxesActor * Viewer::createAxes(double bounds[6], vtkRenderer & renderer)
{
    vtkCubeAxesActor * cubeAxes = vtkCubeAxesActor::New();
    cubeAxes->SetBounds(bounds);
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

void Viewer::ShowInfo(const QStringList & info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItems(info);

    setToolTip(info.join('\n'));
}

void Viewer::setCurrentMainInput(const QString & name)
{
    m_mainRenderer->RemoveAllViewProps();
    for (const vtkSmartPointer<vtkProp> & prop : m_loadedInputs[name]) {
        m_mainRenderer->AddViewProp(prop);
    }
    vtkCamera & camera = *m_mainRenderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_mainRenderer->ResetCamera();
    m_ui->qvtkMain->GetRenderWindow()->Render();
}

void Viewer::on_actionSphere_triggered()
{
    setCurrentMainInput("sphere");
}

void Viewer::on_actionVolcano_triggered()
{
    setCurrentMainInput("volcano");
}

void Viewer::on_actionObservation_triggered()
{
    setCurrentMainInput("displacements");
}
