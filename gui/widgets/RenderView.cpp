#include "RenderView.h"
#include "ui_RenderView.h"

#include <vtkLookupTable.h>

#include <vtkPolyData.h>

#include <vtkElevationFilter.h>

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

#include "PickingInteractionStyle.h"
#include "SelectionHandler.h"
#include "NormalRepresentation.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"


using namespace std;

RenderView::RenderView(
    const DataChooser & dataChooser,
    RenderConfigWidget & renderConfigWidget,
    std::shared_ptr<SelectionHandler> selectionHandler)
: QDockWidget()
, m_ui(new Ui_RenderView())
, m_vertexNormalRepresentation(new NormalRepresentation())
, m_dataChooser(dataChooser)
, m_renderConfigWidget(renderConfigWidget)
, m_selectionHandler(selectionHandler)
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();
    
    m_vertexNormalRepresentation->setVisible(false);
    m_renderer->AddViewProp(m_vertexNormalRepresentation->actor());
    m_renderConfigWidget.addPropertyGroup(m_vertexNormalRepresentation->createPropertyGroup());
    
    connect(m_vertexNormalRepresentation, &NormalRepresentation::geometryChanged, this, &RenderView::render);
}

void RenderView::render()
{
    m_renderer->GetRenderWindow()->Render();
}

void RenderView::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(0);

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_renderer);

    m_renderProperty = vtkProperty::New();
    m_renderProperty->SetColor(0, 0.6, 0);
    m_renderProperty->SetOpacity(1.0);
    m_renderProperty->SetInterpolationToFlat();
    m_renderProperty->SetEdgeVisibility(true);
    m_renderProperty->SetEdgeColor(0.1, 0.1, 0.1);
    m_renderProperty->SetLineWidth(1.2);
    m_renderProperty->SetBackfaceCulling(false);
    m_renderProperty->SetLighting(false);

    m_renderConfigWidget.setRenderProperty(m_renderProperty);
}

void RenderView::setupInteraction()
{
    m_interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    m_interactStyle->SetDefaultRenderer(m_renderer);
    connect(m_interactStyle.Get(), &PickingInteractionStyle::pointInfoSent, this, &RenderView::ShowInfo);
    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_interactor->SetInteractorStyle(m_interactStyle);
    m_interactor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_interactor->Initialize();
}

void RenderView::ShowInfo(const QStringList & info)
{
    setToolTip(info.join('\n'));
}

void RenderView::uiSelectionChanged(int)
{
    applyRenderingConfiguration();
}

void RenderView::updateScalarsForColorMaping(DataSelection /*selection*/)
{
    // just rebuild the graphics for now
    emit applyRenderingConfiguration();
}

void RenderView::updateGradientForColorMapping(const QImage & /*gradient*/)
{
    // just rebuild the graphics for now
    emit applyRenderingConfiguration();
}

void RenderView::applyRenderingConfiguration()
{
    if (m_inputs.empty())
        return;

    if (m_inputs.front()->type != ModelType::triangles)
        return;

    // create the visual representation again, to update to scalar to color mapping
    m_renderer->RemoveAllViewProps();
    show3DInput(std::dynamic_pointer_cast<PolyDataInput>(m_inputs.front()));

    emit render();
}

vtkPolyDataMapper * RenderView::map3DInputScalars(PolyDataInput & input)
{
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();

    switch (m_dataChooser.dataSelection()) {
    case DataSelection::NoSelection:
    case DataSelection::DefaultColor:
        mapper->SetInputData(input.polyData());
        return mapper;
    }

    VTK_CREATE(vtkElevationFilter, elevation);
    elevation->SetInputData(input.polyData());

    float minValue, maxValue;

    switch (m_dataChooser.dataSelection()) {
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


    const QImage & gradient = m_renderConfigWidget.selectedGradient();

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

void RenderView::show3DInput(std::shared_ptr<PolyDataInput> input)
{
    m_renderer->RemoveAllViewProps();
    m_inputs = { input };
    
    vtkSmartPointer<vtkPolyDataMapper> mapper = map3DInputScalars(*input);

    vtkSmartPointer<vtkActor> actor = input->createActor();
    actor->SetMapper(mapper);
    actor->SetProperty(m_renderProperty);

    m_renderer->AddViewProp(actor);

    m_selectionHandler->setDataObject(input->data());

    setupAxes(input->data()->GetBounds());

    m_vertexNormalRepresentation->setData(input->polyData());
    m_renderer->AddViewProp(m_vertexNormalRepresentation->actor());
    
    vtkCamera & camera = *m_renderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_renderer->ResetCamera();
    emit render();
}

void RenderView::showGridInput(std::shared_ptr<GridDataInput> input)
{
    m_renderer->RemoveAllViewProps();
    m_inputs = { input };
    
    VTK_CREATE(vtkScalarBarActor, heatBars);
    heatBars->SetTitle(input->name.c_str());
    heatBars->SetLookupTable(input->lookupTable);
    m_renderer->AddViewProp(heatBars);
    m_renderer->AddViewProp(input->createTexturedPolygonActor());
    m_selectionHandler->setDataObject(input->data());

    setupAxes(input->bounds);
    
    vtkCamera & camera = *m_renderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_renderer->ResetCamera();
    emit render();
}

void RenderView::setupAxes(const double bounds[6])
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

vtkSmartPointer<vtkCubeAxesActor> RenderView::createAxes(vtkRenderer & renderer)
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

vtkRenderWindow * RenderView::renderWindow()
{
    return m_ui->qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * RenderView::renderWindow() const
{
    return m_ui->qvtkMain->GetRenderWindow();
}

vtkProperty * RenderView::renderProperty()
{
    return m_renderProperty;
}

const vtkProperty * RenderView::renderProperty() const
{
    return m_renderProperty;
}

PickingInteractionStyle * RenderView::interactStyle()
{
    return m_interactStyle;
}

const PickingInteractionStyle * RenderView::interactStyle() const
{
    return m_interactStyle;
}
