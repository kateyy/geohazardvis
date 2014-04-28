#include "inputviewer.h"
#include "ui_inputviewer.h"

#include <functional>

#include <QFileDialog>
#include <QMessageBox>
#include <QTableView>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkLookupTable.h>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkPointData.h>

#include <vtkArrowSource.h>

#include <vtkElevationFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkGlyph3D.h>

#include <vtkPolyDataMapper.h>
#include <vtkDataSetMapper.h>

#include <vtkScalarBarActor.h>
#include <vtkCubeAxesActor.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "pickinginteractionstyle.h"

#include "core/vtkhelper.h"
#include "core/loader.h"
#include "core/input.h"

#include "mainwindow.h"
#include "selectionhandler.h"
#include "qvtktablemodel.h"
#include "normalrepresentation.h"
#include "widgets/datachooser.h"
#include "widgets/renderconfigwidget.h"
#include "widgets/tablewidget.h"

using namespace std;

InputViewer::InputViewer(QWidget * parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
, m_ui(new Ui_InputViewer())
, m_tableWidget(new TableWidget())
, m_dataChooser(new DataChooser)
, m_renderConfigWidget(new RenderConfigWidget())
, m_vertexNormalRepresentation(new NormalRepresentation())
{
    m_ui->setupUi(this);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_tableWidget);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_dataChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_renderConfigWidget);

    connect(m_dataChooser, &DataChooser::selectionChanged, this, &InputViewer::updateScalarsForColorMaping);
    connect(m_renderConfigWidget, &RenderConfigWidget::gradientSelectionChanged, this, &InputViewer::updateGradientForColorMapping);

    setupRenderer();
    setupInteraction();

    m_selectionHandler = make_shared<SelectionHandler>();
    m_selectionHandler->setQtTableView(m_tableWidget->tableView(), m_tableWidget->model());
    m_selectionHandler->setVtkInteractionStyle(m_interactStyle);

    connect(m_ui->dockWindowButton, &QPushButton::released, this, &InputViewer::dockingRequested);

    m_vertexNormalRepresentation->setVisible(false);
    m_mainRenderer->AddViewProp(m_vertexNormalRepresentation->actor());
    m_renderConfigWidget->addPropertyGroup(m_vertexNormalRepresentation->createPropertyGroup());

    auto renderFunc = std::bind(&vtkRenderWindow::Render, m_mainRenderer->GetRenderWindow());
    connect(m_vertexNormalRepresentation, &NormalRepresentation::geometryChanged, renderFunc);
    connect(m_renderConfigWidget, &RenderConfigWidget::renderPropertyChanged, renderFunc);
}

InputViewer::~InputViewer()
{
    delete m_ui;
}

bool InputViewer::isEmpty() const
{
    return m_inputs.empty();
}

void InputViewer::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(0);

    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    m_renderProperty = vtkProperty::New();
    m_renderProperty->SetColor(0, 0.6, 0);
    m_renderProperty->SetOpacity(1.0);
    m_renderProperty->SetInterpolationToFlat();
    m_renderProperty->SetEdgeVisibility(true);
    m_renderProperty->SetEdgeColor(0.1, 0.1, 0.1);
    m_renderProperty->SetLineWidth(1.2);
    m_renderProperty->SetBackfaceCulling(false);
    m_renderProperty->SetLighting(false);

    m_renderConfigWidget->setRenderProperty(m_renderProperty);
}

void InputViewer::setupInteraction()
{
    m_interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    m_interactStyle->SetDefaultRenderer(m_mainRenderer);
    connect(m_interactStyle.Get(), &PickingInteractionStyle::pointInfoSent, this, &InputViewer::ShowInfo);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(m_interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
}

void InputViewer::showEvent(QShowEvent * event)
{
    // show the docking button only if we are a floating window
    m_ui->dockWindowButton->setVisible((windowFlags() & Qt::Window) == Qt::Window);
}

void InputViewer::dragEnterEvent(QDragEnterEvent * event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void InputViewer::dropEvent(QDropEvent * event)
{
    assert(event->mimeData()->hasUrls());
    QString filename = event->mimeData()->urls().first().toLocalFile();
    qDebug() << filename;

    emit openFile(filename);

    event->acceptProposedAction();
}

void InputViewer::ShowInfo(const QStringList & info)
{
    m_ui->qvtkMain->setToolTip(info.join('\n'));
}

void InputViewer::uiSelectionChanged(int)
{
    applyRenderingConfiguration();
}

void InputViewer::updateScalarsForColorMaping(DataSelection /*selection*/)
{
    // just rebuild the graphics for now
    emit applyRenderingConfiguration();
}

void InputViewer::updateGradientForColorMapping(const QImage & /*gradient*/)
{
    // just rebuild the graphics for now
    emit applyRenderingConfiguration();
}

void InputViewer::applyRenderingConfiguration()
{
    if (m_inputs.empty())
        return;

    if (m_inputs.front()->type != ModelType::triangles)
        return;

    // create the visual representation again, to update to scalar to color mapping
    m_mainRenderer->RemoveAllViewProps();
    show3DInput(static_cast<PolyDataInput&>(*m_inputs.front()));

    m_ui->qvtkMain->GetRenderWindow()->Render();
}

void InputViewer::openFile(QString filename)
{
    QApplication::processEvents();

    QString oldName = windowTitle();
    setWindowTitle(filename + " (loading file)");
    QApplication::processEvents();

    shared_ptr<Input> input = Loader::readFile(filename.toStdString());
    if (!input) {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        setWindowTitle(oldName);
        return;
    }

    setWindowTitle(QString::fromStdString(input->name) + " (loading to gpu)");
    QApplication::processEvents();

    m_inputs = { input };

    m_mainRenderer->RemoveAllViewProps();

    switch (input->type) {
    case ModelType::triangles:
        show3DInput(static_cast<PolyDataInput&>(*input));
        break;
    case ModelType::grid2d:
        showGridInput(static_cast<GridDataInput&>(*input));
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

    setWindowTitle(QString::fromStdString(input->name));
    QApplication::processEvents();
}

vtkPolyDataMapper * InputViewer::map3DInputScalars(PolyDataInput & input)
{
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();

    switch (m_dataChooser->dataSelection()) {
    case DataSelection::NoSelection:
    case DataSelection::DefaultColor:
        mapper->SetInputData(input.polyData());
        return mapper;
    }

    VTK_CREATE(vtkElevationFilter, elevation);
    elevation->SetInputData(input.polyData());

    float minValue, maxValue;

    switch (m_dataChooser->dataSelection()) {
    case DataSelection::Vertex_xValues:
        minValue = input.polyData()->GetBounds()[0];
        maxValue = input.polyData()->GetBounds()[1];
        elevation->SetLowPoint(minValue, 0, 0);
        elevation->SetHighPoint(maxValue, 0, 0);
        break;

    case DataSelection::Vertex_yValues:
        minValue = input.polyData()->GetBounds()[2];
        maxValue = input.polyData()->GetBounds()[3];
        elevation->SetLowPoint(0, minValue, 0);
        elevation->SetHighPoint(0, maxValue, 0);
        break;

    case DataSelection::Vertex_zValues:
        minValue = input.polyData()->GetBounds()[4];
        maxValue = input.polyData()->GetBounds()[5];
        elevation->SetLowPoint(0, 0, minValue);
        elevation->SetHighPoint(0, 0, maxValue);
        break;
    }

    mapper->SetInputConnection(elevation->GetOutputPort());


    const QImage & gradient = m_renderConfigWidget->selectedGradient();

    // use alpha = 1.0, if the image doesn't have a alpha channel
    int alphaMask = gradient.hasAlphaChannel() ? 0x00 : 0xFF;

    VTK_CREATE(vtkLookupTable, lut);
    lut->SetNumberOfTableValues(gradient.width());
    for (int i = 0; i < gradient.width(); ++i) {
        QRgb color = gradient.pixel(i, 0);
        lut->SetTableValue(i, qRed(color) / 255.0, qGreen(color) / 255.0, qBlue(color) / 255.0, (alphaMask | qAlpha(color)) / 255.0);
    }
    lut->SetValueRange(minValue, maxValue);

    mapper->SetLookupTable(lut);

    return mapper;
}

void InputViewer::show3DInput(PolyDataInput & input)
{
    vtkSmartPointer<vtkPolyDataMapper> mapper = map3DInputScalars(input);

    vtkSmartPointer<vtkActor> actor = input.createActor();
    actor->SetMapper(mapper);
    actor->SetProperty(m_renderProperty);

    m_mainRenderer->AddViewProp(actor);

    m_selectionHandler->setDataObject(input.data());

    setupAxes(input.data()->GetBounds());

    m_vertexNormalRepresentation->setData(input.polyData());
    m_mainRenderer->AddViewProp(m_vertexNormalRepresentation->actor());

    m_tableWidget->showData(input.polyData());
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

    m_tableWidget->showData(input.data());
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
