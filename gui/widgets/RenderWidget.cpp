#include "RenderWidget.h"
#include "ui_RenderWidget.h"

#include <QMessageBox>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkLookupTable.h>

#include <vtkPolyData.h>

#include <vtkPolyDataMapper.h>

#include <vtkScalarBarActor.h>
#include <vtkCubeAxesActor.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>

#include "core/vtkhelper.h"
#include "core/Input.h"
#include "core/data_objects/PolyDataObject.h"
#include "core/data_objects/ImageDataObject.h"
#include "core/data_objects/RenderedPolyData.h"
#include "core/data_objects/RenderedImageData.h"

#include "PickingInteractionStyle.h"
#include "SelectionHandler.h"
#include "NormalRepresentation.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"


using namespace std;

RenderWidget::RenderWidget(
    int index,
    DataChooser & dataChooser,
    RenderConfigWidget & renderConfigWidget,
    SelectionHandler & selectionHandler)
: QDockWidget()
, m_ui(new Ui_RenderWidget())
, m_index(index)
, m_dataChooser(dataChooser)
, m_renderConfigWidget(renderConfigWidget)
, m_selectionHandler(selectionHandler)
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();

    updateWindowTitle();

    connect(&m_dataChooser, &DataChooser::renderSetupChanged, this, &RenderWidget::render);
}

RenderWidget::~RenderWidget()
{
    m_renderConfigWidget.clear();

    if (m_dataChooser.mapping() == &m_scalarMapping)
        m_dataChooser.setMapping();

    qDeleteAll(m_renderedData);
}

int RenderWidget::index() const
{
    return m_index;
}

void RenderWidget::render()
{
    m_renderer->GetRenderWindow()->Render();
}

void RenderWidget::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(0);

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_renderer);
}

void RenderWidget::setupInteraction()
{
    m_interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    m_interactStyle->SetDefaultRenderer(m_renderer);

    connect(m_interactStyle.Get(), &PickingInteractionStyle::pointInfoSent, this, &RenderWidget::ShowInfo);
    connect(m_interactStyle.Get(), &PickingInteractionStyle::actorPicked, this, &RenderWidget::on_actorPicked);

    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_interactor->SetInteractorStyle(m_interactStyle);
    m_interactor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_interactor->Initialize();
}

void RenderWidget::ShowInfo(const QStringList & info)
{
    setToolTip(info.join('\n'));
}

void RenderWidget::addDataObject(DataObject * dataObject)
{
    setWindowTitle(QString::fromStdString(dataObject->input()->name) + " (loading to GPU)");
    QApplication::processEvents();

    if (!m_renderedData.empty())
    {
        if (m_renderedData.first()->dataObject()->input()->type != dataObject->input()->type)
        {
            QMessageBox::warning(this, "", "Cannot render 2d and 3d geometry in the same view.");
            return;
        }
    }

    RenderedData * renderedData;

    switch (dataObject->input()->type)
    {
    case ModelType::triangles:
    {
        RenderedPolyData * renderedPolyData = new RenderedPolyData(dynamic_cast<PolyDataObject*>(dataObject));
        show3DInput(renderedPolyData);
        renderedData = renderedPolyData;
        break;
    }
    case ModelType::grid2d:
    {
        assert(false);
        /*RenderedImageData * renderedImageData = new RenderedImageData(dynamic_cast<ImageDataObject&>(*dataObject));
        showGridInput(renderedImageData);
        renderedData = renderedImageData;*/
        break;
    }
    default:
        assert(false);
    }

    m_renderedData << renderedData;

    double bounds[6] = {
        std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest() };

    for (const auto & rendered : m_renderedData)
    {
        for (int i = 0; i < 6; i += 2)
        {
            const double * inputBounds = rendered->dataObject()->input()->bounds();
            bounds[i] = std::min(bounds[i], inputBounds[i]);
            bounds[i + 1] = std::max(bounds[i + 1], inputBounds[i + 1]);
        }
    }
    setupAxes(bounds);

    updateWindowTitle();

    m_scalarMapping.setRenderedData(m_renderedData);

    m_renderConfigWidget.setRenderedData(renderedData);
    m_dataChooser.setMapping(windowTitle(), &m_scalarMapping);

    vtkCamera & camera = *m_renderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_renderer->ResetCamera();
    emit render();
}

void RenderWidget::setDataObject(DataObject * dataObject)
{
    m_renderer->RemoveAllViewProps();

    m_renderedData.clear();
    addDataObject(dataObject);
}

void RenderWidget::show3DInput(RenderedPolyData * renderedPolyData)
{
    vtkActor * actor = renderedPolyData->actor();

    m_actorToRenderedData.insert(actor, renderedPolyData);

    m_renderer->AddViewProp(actor);

    //m_vertexNormalRepresentation->setData(input->polyData());
    //m_renderer->AddViewProp(m_vertexNormalRepresentation->actor());
}

void RenderWidget::showGridInput(RenderedImageData * renderedImageData)
{
    const auto input = renderedImageData->imageDataObject()->gridDataInput();
    VTK_CREATE(vtkScalarBarActor, heatBars);
    heatBars->SetTitle(input->name.c_str());
    heatBars->SetLookupTable(input->lookupTable);
    m_renderer->AddViewProp(heatBars);
    m_renderer->AddViewProp(renderedImageData->actor());
    m_selectionHandler.setDataObject(input->data());
}

void RenderWidget::setupAxes(const double bounds[6])
{
    if (!m_axesActor) {
        m_axesActor = createAxes(*m_renderer);
    }
    double b[6];
    for (int i = 0; i < 6; ++i)
        b[i] = bounds[i];
    m_renderer->AddViewProp(m_axesActor);
    m_axesActor->SetBounds(b);
    m_axesActor->SetRebuildAxes(true);
}

vtkSmartPointer<vtkCubeAxesActor> RenderWidget::createAxes(vtkRenderer & renderer)
{
    VTK_CREATE(vtkCubeAxesActor, cubeAxes);
    cubeAxes->SetCamera(m_renderer->GetActiveCamera());
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

void RenderWidget::updateWindowTitle()
{
    QString title;
    for (const auto & renderedData : m_renderedData)
    {
        title += ", " + QString::fromStdString(renderedData->dataObject()->input()->name);
    }
    if (title.isEmpty())
        title = "(empty)";
    else
        title.remove(0, 2);

    title = QString::number(index()) + ": " + title;

    setWindowTitle(title);
}

vtkRenderWindow * RenderWidget::renderWindow()
{
    return m_ui->qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * RenderWidget::renderWindow() const
{
    return m_ui->qvtkMain->GetRenderWindow();
}

PickingInteractionStyle * RenderWidget::interactStyle()
{
    return m_interactStyle;
}

const PickingInteractionStyle * RenderWidget::interactStyle() const
{
    return m_interactStyle;
}

void RenderWidget::closeEvent(QCloseEvent * event)
{
    emit closed();

    QDockWidget::closeEvent(event);
}

void RenderWidget::on_actorPicked(vtkActor * actor)
{
    assert(actor);

    vtkInformation * inputInfo = actor->GetMapper()->GetInformation();

    QString propertyName;
    if (inputInfo->Has(Input::NameKey()))
        propertyName = Input::NameKey()->Get(inputInfo);

    m_renderConfigWidget.setRenderedData(m_actorToRenderedData[actor]);
    m_dataChooser.setMapping(windowTitle(), &m_scalarMapping);
}