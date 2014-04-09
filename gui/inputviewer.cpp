#include "inputviewer.h"
#include "ui_inputviewer.h"

#include <QFileDialog>
#include <QMessageBox>

#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkLookupTable.h>

#include <vtkDataSet.h>

#include <vtkScalarBarActor.h>
#include <vtkCubeAxesActor.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "pickinginteractionstyle.h"

#include "core/vtkhelper.h"
#include "core/loader.h"
#include "core/input.h"

#include "mainwindow.h"
#include "renderwidget.h"
#include "selectionhandler.h"
#include "qvtktablemodel.h"

using namespace std;

InputViewer::InputViewer(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_InputViewer())
, m_tableModel(nullptr)
{
    m_ui->setupUi(this);

    m_tableModel = new QVtkTableModel(m_ui->tableView);
    QItemSelectionModel * m = m_ui->tableView->selectionModel();
    m_ui->tableView->setModel(m_tableModel);
    m->deleteLater();

    setupRenderer();
    setupInteraction();

    m_selectionHandler = make_shared<SelectionHandler>();
    m_selectionHandler->setQtTableView(m_ui->tableView, m_tableModel);
    m_selectionHandler->setVtkInteractionStyle(m_interactStyle);
}

InputViewer::~InputViewer()
{
    delete m_ui;
}

void InputViewer::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(2);

    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    connect(m_ui->qvtkMain, &RenderWidget::onInputFileDropped, this, &InputViewer::openFile);
}

void InputViewer::setupInteraction()
{
    m_interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    m_interactStyle->SetDefaultRenderer(m_mainRenderer);
    connect(m_interactStyle, &PickingInteractionStyle::pointInfoSent, this, &InputViewer::ShowInfo);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(m_interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
}

void InputViewer::ShowInfo(const QStringList & info)
{
    m_ui->qvtkMain->setToolTip(info.join('\n'));
}

void InputViewer::openFile(QString filename)
{
    shared_ptr<Input> input = Loader::readFile(filename.toStdString());
    if (!input) {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        return;
    }

    setWindowTitle(QString::fromStdString(input->name));

    m_inputs = { input };

    m_mainRenderer->RemoveAllViewProps();

    switch (input->type) {
    case ModelType::triangles:
        show3DInput(*static_cast<PolyDataInput*>(input.get()));
        break;
    case ModelType::grid2d:
        showGridInput(*static_cast<GridDataInput*>(input.get()));
        break;
    default:
        QMessageBox::critical(this, "File error", "Could not open the selected input file. (unsupported format)");
        return;
    }

    vtkCamera & camera = *m_mainRenderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_mainRenderer->ResetCamera();
    m_ui->qvtkMain->GetRenderWindow()->Render();
}

void InputViewer::show3DInput(PolyDataInput & input)
{
    vtkSmartPointer<vtkActor> actor = input.createActor();
    vtkProperty & prop = *actor->GetProperty();
    prop.SetColor(0, 0.6, 0);
    prop.SetOpacity(1.0);
    prop.SetInterpolationToGouraud();
    prop.SetEdgeVisibility(true);
    prop.SetEdgeColor(0.1, 0.1, 0.1);
    prop.SetLineWidth(1.2);
    prop.SetBackfaceCulling(false);
    prop.SetLighting(true);

    m_mainRenderer->AddViewProp(actor);

    m_selectionHandler->setDataObject(input.data());

    setupAxes(input.data()->GetBounds());

    m_tableModel->showPolyData(input.polyData());
    m_ui->tableView->resizeColumnsToContents();
}

void InputViewer::showGridInput(GridDataInput & input)
{
    VTK_CREATE(vtkScalarBarActor, heatBars);
    heatBars->SetTitle(input.name.c_str());
    heatBars->SetLookupTable(input.lookupTable);
    m_mainRenderer->AddViewProp(heatBars);
    m_mainRenderer->AddViewProp(input.createTexturedPolygonActor());
    m_selectionHandler->setDataObject(input.data());

    setupAxes(input.bounds);

    m_tableModel->showGridData(input.imageData());
    m_ui->tableView->resizeColumnsToContents();
}

void InputViewer::setupAxes(const double bounds[6])
{
    if (!m_axesActor) {
        m_axesActor = createAxes(*m_mainRenderer);
    }
    double b[6];
    for (int i = 0; i < 6; ++i)
        b[i] = bounds[i];
    m_mainRenderer->AddViewProp(m_axesActor);
    m_axesActor->SetBounds(b);
    m_axesActor->SetRebuildAxes(true);
}

vtkSmartPointer<vtkCubeAxesActor> InputViewer::createAxes(vtkRenderer & renderer)
{
    VTK_CREATE(vtkCubeAxesActor, cubeAxes);
    cubeAxes->SetCamera(m_mainRenderer->GetActiveCamera());
    cubeAxes->SetFlyModeToOuterEdges();
    cubeAxes->SetEnableDistanceLOD(1);
    cubeAxes->SetEnableViewAngleLOD(1);
    cubeAxes->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);

    double axesColor[3] = { 0, 0, 0 };
    double gridColor[3] = { 0.7, 0.7, 0.7 };

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
