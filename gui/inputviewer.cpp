#include "inputviewer.h"
#include "ui_inputviewer.h"

#include <QFileDialog>
#include <QMessageBox>
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

using namespace std;

InputViewer::InputViewer(QWidget * parent)
: QWidget(parent)
, m_ui(new Ui_InputViewer())
, m_tableModel(nullptr)
{
    m_ui->setupUi(this);

    m_tableModel = new QVtkTableModel(m_ui->tableView);
    m_ui->tableView->setModel(m_tableModel);

    setupRenderer();
    setupInteraction();
    loadGradientImages();

    m_selectionHandler = make_shared<SelectionHandler>();
    m_selectionHandler->setQtTableView(m_ui->tableView, m_tableModel);
    m_selectionHandler->setVtkInteractionStyle(m_interactStyle);

    connect(m_ui->dockWindowButton, &QPushButton::released, this, &InputViewer::dockingRequested);
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

void InputViewer::loadGradientImages()
{
    // navigate to the gradient directory
    QDir dir;
    if (!dir.cd("data/gradients"))
    {
        std::cout << "gradient directory does not exist; no gradients will be available" << std::endl;
        return;
    }

    // only retrieve png and jpeg files
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg";
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();

    QComboBox* gradientComboBox = m_ui->gradientComboBox;
    // load the files and add them to the combobox
    gradientComboBox->blockSignals(true);

    for (QFileInfo fileInfo : list)
    {
        QString fileName = fileInfo.baseName();
        QString filePath = fileInfo.absoluteFilePath();
        QPixmap pixmap = QPixmap(filePath).scaled(200, 20);
        m_scalarToColorGradients << pixmap.toImage();
        
        gradientComboBox->addItem(pixmap, "");
        QVariant fileVariant(filePath);
        gradientComboBox->setItemData(gradientComboBox->count() - 1, fileVariant, Qt::UserRole);
    }

    gradientComboBox->setIconSize(QSize(200, 20));
    gradientComboBox->blockSignals(false);
    // set the "default" gradient
    gradientComboBox->setCurrentIndex(34);
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
    updateScalarToColorMapping();
}

void InputViewer::updateScalarToColorMapping()
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
    VTK_CREATE(vtkElevationFilter, elevation);
    elevation->SetInputData(input.polyData());

    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();

    if (m_ui->scalars_singleColor->isChecked()) {
        mapper->SetInputData(input.polyData());
        return mapper;
    }

    float minValue, maxValue;

    if (m_ui->scalars_xValues->isChecked()) {
        minValue = input.polyData()->GetBounds()[0];
        maxValue = input.polyData()->GetBounds()[1];
        elevation->SetLowPoint(minValue, 0, 0);
        elevation->SetHighPoint(maxValue, 0, 0);
    }
    else if (m_ui->scalars_yValues->isChecked())
    {
        minValue = input.polyData()->GetBounds()[2];
        maxValue = input.polyData()->GetBounds()[3];
        elevation->SetLowPoint(0, minValue, 0);
        elevation->SetHighPoint(0, maxValue, 0);
    }
    else if (m_ui->scalars_zValues->isChecked())
    {
        minValue = input.polyData()->GetBounds()[4];
        maxValue = input.polyData()->GetBounds()[5];
        elevation->SetLowPoint(0, 0, minValue);
        elevation->SetHighPoint(0, 0, maxValue);
    }
    else
    {
        assert(false);
    }

    mapper->SetInputConnection(elevation->GetOutputPort());


    const QImage & gradient = m_scalarToColorGradients[m_ui->gradientComboBox->currentIndex()];

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

    vtkProperty & prop = *actor->GetProperty();
    prop.SetColor(0, 0.6, 0);
    prop.SetOpacity(1.0);
    prop.SetInterpolationToFlat();
    prop.SetEdgeVisibility(true);
    prop.SetEdgeColor(0.1, 0.1, 0.1);
    prop.SetLineWidth(1.2);
    prop.SetBackfaceCulling(false);
    prop.SetLighting(false);

    m_mainRenderer->AddViewProp(actor);

    m_selectionHandler->setDataObject(input.data());

    setupAxes(input.data()->GetBounds());

    showVertexNormals(input.polyData());

    m_tableModel->showPolyData(input.polyData());
    m_ui->tableView->resizeColumnsToContents();
}

void InputViewer::showVertexNormals(vtkPolyData * polyData)
{
    VTK_CREATE(vtkPolyDataNormals, inputNormals);
    inputNormals->ComputeCellNormalsOff();
    inputNormals->ComputePointNormalsOn();
    inputNormals->SetInputDataObject(polyData);

    VTK_CREATE(vtkArrowSource, arrow);
    arrow->SetShaftRadius(0.02);
    arrow->SetTipRadius(0.07);
    arrow->SetTipLength(0.3);

    VTK_CREATE(vtkGlyph3D, arrowGlyph);

    double * bounds = polyData->GetBounds();
    double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));

    arrowGlyph->SetScaleModeToScaleByScalar();
    arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);
    arrowGlyph->SetVectorModeToUseNormal();
    arrowGlyph->OrientOn();
    arrowGlyph->SetInputConnection(inputNormals->GetOutputPort());
    arrowGlyph->SetSourceConnection(arrow->GetOutputPort());

    VTK_CREATE(vtkDataSetMapper, arrowMapper);
    arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());
    VTK_CREATE(vtkActor, arrowActor);
    arrowActor->SetMapper(arrowMapper);

    m_mainRenderer->AddViewProp(arrowActor);
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
