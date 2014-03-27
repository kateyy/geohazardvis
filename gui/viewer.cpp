#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

#include <QDebug>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkCamera.h>
#include <vtkLookupTable.h>

// inputs
#include <vtkPolyDataAlgorithm.h>
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
#include "renderwidget.h"
#include <QFileDialog>
#include <QMessageBox>

#include "core/loader.h"
#include "core/input.h"
#include "qvtktablemodel.h"


using namespace std;

#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


Viewer::Viewer()
: m_ui(new Ui_Viewer())
, m_tableModel(nullptr)
{
    m_ui->setupUi(this);
    m_tableModel = new QVtkTableModel;
    m_ui->tableView->setModel(m_tableModel);

    setupRenderer();
    setupInteraction();
}

Viewer::~Viewer()
{
    delete m_ui;
}

void Viewer::setupRenderer()
{
    //m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(2);

    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    connect(m_ui->qvtkMain, &RenderWidget::onInputFileDropped, this, &Viewer::openFile);
}

void Viewer::setupInteraction()
{
    m_interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    m_interactStyle->SetDefaultRenderer(m_mainRenderer);
    connect(m_interactStyle, &PickingInteractionStyle::pointInfoSent, this, &Viewer::ShowInfo);
    connect(m_interactStyle, &PickingInteractionStyle::selectionChanged, this, &Viewer::selectPoint);
    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, m_interactStyle, &PickingInteractionStyle::changeSelection);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(m_interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
}

void Viewer::ShowInfo(const QStringList & info)
{
    setToolTip(info.join('\n'));
}

void Viewer::selectPoint(int index)
{
    m_ui->tableView->selectRow(index);
}

void Viewer::on_actionOpen_triggered()
{
    static QString lastFolder;
    QString filename = QFileDialog::getOpenFileName(this, "", lastFolder, "Text files (*.txt)");
    if (filename.isEmpty())
        return;

    lastFolder = QFileInfo(filename).absolutePath();

    emit openFile(filename);
}

void Viewer::openFile(QString filename)
{
    shared_ptr<Input> input = Loader::readFile(filename.toStdString());
    if (!input) {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        return;
    }

    m_inputs = {input};

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

void Viewer::show3DInput(PolyDataInput & input)
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
    m_interactStyle->setMainDataObject(input.data());

    setupAxes(input.data()->GetBounds());

    m_tableModel->showPolyData(input.polyData());
    m_ui->tableView->resizeColumnsToContents();
}

void Viewer::showGridInput(GridDataInput & input)
{
    VTK_CREATE(vtkScalarBarActor, heatBars);
    heatBars->SetTitle(input.name.c_str());
    heatBars->SetLookupTable(input.lookupTable);
    m_mainRenderer->AddViewProp(heatBars);
    m_mainRenderer->AddViewProp(input.createTexturedPolygonActor());
    m_interactStyle->setMainDataObject(input.data());

    setupAxes(input.bounds);
}

void Viewer::setupAxes(const double bounds[6])
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

vtkSmartPointer<vtkCubeAxesActor> Viewer::createAxes(vtkRenderer & renderer)
{
    VTK_CREATE(vtkCubeAxesActor, cubeAxes);
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
