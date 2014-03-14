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

#include <QAbstractTableModel>
#include <QVariant>

#include "core/loader.h"
#include "core/input.h"

using namespace std;


#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


class QVtkTableModel : public QAbstractTableModel {
public:
    QVtkTableModel(QObject * parent = nullptr)
    : QAbstractTableModel(parent)
    {
    }
    ~QVtkTableModel() override {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (m_vtkData == nullptr)
            return 0;
        return m_vtkData->GetPoints()->GetNumberOfPoints();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        if (m_vtkData == nullptr)
            return 0;
        return 4;
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        assert(index.column() < 4);
        if (!index.isValid() || role != Qt::DisplayRole)
            return QVariant();
        if (index.column() == 0)
            return QVariant(index.row());
        double * vertex = m_vtkData->GetPoints()->GetPoint(index.row());
        return QVariant(vertex[index.column()-1]);
    }
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Orientation::Vertical)
            return QVariant();
        if (role != Qt::DisplayRole)
            return QVariant();
        if (section == 0)
            return QVariant("index");
        return QVariant(QChar('x' + section - 1));
    }

    void setVtkData(vtkSmartPointer<vtkPolyData> data) {
        assert(m_vtkData == nullptr);
        /*beginInsertColumns(QModelIndex(), 0, 3);
        beginInsertRows(QModelIndex(), 0, data->GetPoints()->GetNumberOfPoints() - 1);*/
        beginResetModel();
        m_vtkData = data;
        /*endInsertRows();
        endInsertColumns();*/
        endResetModel();
    }

protected:
    vtkSmartPointer<vtkPolyData> m_vtkData;
};


Viewer::Viewer()
: m_ui(new Ui_Viewer())
{
    m_ui->setupUi(this);
    QVtkTableModel * model = new QVtkTableModel;
    m_ui->tableView->setModel(model);

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
    VTK_CREATE(PickingInteractionStyle, interactStyle);
    interactStyle->SetDefaultRenderer(m_mainRenderer);
    connect(&interactStyle->pickingInfo, &PickingInfo::infoSent, this, &Viewer::ShowInfo);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
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

    setupAxes(input.data()->GetBounds());

    ((QVtkTableModel*) m_ui->tableView->model())->setVtkData(input.polyData());
    m_ui->tableView->update();
}

void Viewer::showGridInput(GridDataInput & input)
{
    VTK_CREATE(vtkScalarBarActor, heatBars);
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
