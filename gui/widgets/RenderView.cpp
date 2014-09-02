#include "RenderView.h"
#include "ui_RenderView.h"

#include <QMessageBox>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkPolyData.h>

#include <vtkPolyDataMapper.h>

#include <vtkCubeAxesActor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkProperty2D.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>

#include <core/vtkhelper.h>
#include <core/Input.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/RenderedPolyData.h>
#include <core/data_objects/RenderedImageData.h>

#include "PickingInteractorStyleSwitch.h"
#include "InteractorStyle3D.h"
#include "InteractorStyleImage.h"
#include "SelectionHandler.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"


using namespace std;


RenderView::RenderView(
    int index,
    DataChooser & dataChooser,
    RenderConfigWidget & renderConfigWidget,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_ui(new Ui_RenderView())
    , m_dataChooser(dataChooser)
    , m_renderConfigWidget(renderConfigWidget)
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();
    setupColorMappingLegend();

    updateWindowTitle();

    connect(&m_dataChooser, &DataChooser::renderSetupChanged, this, &RenderView::render);

    SelectionHandler::instance().addRenderView(this);
}

RenderView::~RenderView()
{
    SelectionHandler::instance().removeRenderView(this);

    m_renderConfigWidget.clear();

    if (m_dataChooser.mapping() == &m_scalarMapping)
        m_dataChooser.setMapping();

    qDeleteAll(m_renderedData);
}

bool RenderView::isTable() const
{
    return false;
}

bool RenderView::isRenderer() const
{
    return true;
}

void RenderView::render()
{
    m_renderer->GetRenderWindow()->Render();
}

QWidget * RenderView::contentWidget()
{
    return m_ui->qvtkMain;
}

void RenderView::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(0);

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_renderer);
}

void RenderView::setupInteraction()
{
    m_interactorStyle = vtkSmartPointer<PickingInteractorStyleSwitch>::New();
    m_interactorStyle->SetDefaultRenderer(m_renderer);
    m_interactorStyle->setRenderedDataList(&m_renderedData);

    m_interactorStyle->addStyle("InteractorStyle3D", InteractorStyle3D::New());
    m_interactorStyle->addStyle("InteractorStyleImage", InteractorStyleImage::New());

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::pointInfoSent, this, &RenderView::ShowInfo);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::actorPicked, this, &RenderView::updateGuiForActor);
 
    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_interactor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());
    m_interactor->SetInteractorStyle(m_interactorStyle);

    m_interactor->Initialize();
}

void RenderView::setInteractorStyle(const std::string & name)
{
    m_interactorStyle->setCurrentStyle(name);
}

void RenderView::ShowInfo(const QStringList & info)
{
    setToolTip(info.join('\n'));
}

RenderedData * RenderView::addDataObject(DataObject * dataObject)
{
    setWindowTitle(QString::fromStdString(dataObject->input()->name) + " (loading to GPU)");
    QApplication::processEvents();

    if (!m_renderedData.empty()
        && m_renderedData.first()->dataObject()->input()->type != dataObject->input()->type)
    {
        QMessageBox::warning(this, "", "Cannot render 2d and 3d geometry in the same view.");
        updateWindowTitle();
        return nullptr;
    }

    RenderedData * renderedData = nullptr;

    switch (dataObject->input()->type)
    {
    case ModelType::triangles:
        renderedData = new RenderedPolyData(dynamic_cast<PolyDataObject*>(dataObject));
        setInteractorStyle("InteractorStyle3D");
        break;
    case ModelType::grid2d:
        renderedData = new RenderedImageData(dynamic_cast<ImageDataObject*>(dataObject));
        setInteractorStyle("InteractorStyleImage");
        break;
    default:
        assert(false);
    }

    assert(renderedData);

    for (vtkActor * actor : renderedData->actors())
        m_renderer->AddViewProp(actor);

    m_actorToRenderedData.insert(renderedData->mainActor(), renderedData);
    m_renderedData << renderedData;

    connect(renderedData, &RenderedData::geometryChanged, this, &RenderView::render);

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

    m_dataObjectToRendered.insert(dataObject, renderedData);

    return renderedData;
}

void RenderView::addDataObjects(QList<DataObject *> dataObjects)
{
    RenderedData * aNewObject = nullptr;

    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * oldRendered = m_dataObjectToRendered.value(dataObject);

        if (oldRendered)
            oldRendered->setVisible(true);
        else
            aNewObject = addDataObject(dataObject);
    }

    if (aNewObject)
        m_renderConfigWidget.setRenderedData(aNewObject);

    m_scalarMapping.setRenderedData(m_renderedData);
    m_dataChooser.setMapping(windowTitle(), &m_scalarMapping);

    updateWindowTitle();

    vtkCamera & camera = *m_renderer->GetActiveCamera();
    camera.SetPosition(0, 0, 1);
    camera.SetViewUp(0, 1, 0);
    m_renderer->ResetCamera();

    render();
}

void RenderView::hideDataObjects(QList<DataObject *> dataObjects)
{
    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * rendered = m_dataObjectToRendered.value(dataObject);
        if (!rendered)
            continue;

        rendered->setVisible(false);
    }

    render();
}

bool RenderView::isVisible(DataObject * dataObject) const
{
    RenderedData * renderedData = m_dataObjectToRendered.value(dataObject, nullptr);
    if (!renderedData)
        return false;
    return renderedData->isVisible();
}

void RenderView::removeDataObject(DataObject * dataObject)
{
    RenderedData * renderedData = nullptr;
    for (RenderedData * r : m_renderedData)
    {
        if (r->dataObject() == dataObject)
        {
            renderedData = r;
            break;
        }
    }

    // we didn't render this object
    if (!renderedData)
        return;

    // this was our only object, so end here
    if (m_renderedData.size() == 1)
    {
        close();
        return;
    }

    for (vtkActor * actor : renderedData->actors())
        m_renderer->RemoveViewProp(actor);

    m_renderedData.removeOne(renderedData);

    m_scalarMapping.setRenderedData(m_renderedData);
    if (m_renderConfigWidget.renderedData() == renderedData)
        m_renderConfigWidget.setRenderedData(nullptr);
    m_dataChooser.setMapping(windowTitle(), &m_scalarMapping);

    delete renderedData;
}

void RenderView::removeDataObjects(QList<DataObject *> dataObjects)
{
    // TODO optimize as needed
    for (DataObject * dataObject : dataObjects)
        removeDataObject(dataObject);
}

QList<DataObject *> RenderView::dataObjects() const
{
    return m_dataObjectToRendered.keys();
}

QList<const RenderedData *> RenderView::renderedData() const
{
    QList<const RenderedData *> l;
    for (auto r : m_renderedData)
        l << r;
    return l;
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
    cubeAxes->SetCamera(renderer.GetActiveCamera());
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

void RenderView::setupColorMappingLegend()
{
    m_colorMappingLegend = m_scalarMapping.colorMappingLegend();
    m_colorMappingLegend->SetAnnotationTextScaling(false);
    m_colorMappingLegend->SetBarRatio(0.2);
    m_colorMappingLegend->SetNumberOfLabels(7);
    m_colorMappingLegend->SetDrawBackground(true);
    m_colorMappingLegend->GetBackgroundProperty()->SetColor(1, 1, 1);
    m_colorMappingLegend->SetDrawFrame(true);
    m_colorMappingLegend->GetFrameProperty()->SetColor(0, 0, 0);
    m_colorMappingLegend->SetVerticalTitleSeparation(5);
    m_colorMappingLegend->SetTextPad(3);

    vtkTextProperty * labelProp = m_colorMappingLegend->GetLabelTextProperty();
    labelProp->SetShadow(false);
    labelProp->SetColor(0, 0, 0);
    labelProp->SetBold(false);
    labelProp->SetItalic(false);

    vtkTextProperty * titleProp = m_colorMappingLegend->GetTitleTextProperty();
    titleProp->SetShadow(false);
    titleProp->SetColor(0, 0, 0);
    titleProp->SetBold(false);
    titleProp->SetItalic(false);

    VTK_CREATE(vtkScalarBarRepresentation, repr);
    repr->SetScalarBarActor(m_colorMappingLegend);

    m_scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
    m_scalarBarWidget->SetScalarBarActor(m_colorMappingLegend);
    m_scalarBarWidget->SetRepresentation(repr);
    m_scalarBarWidget->SetInteractor(m_interactor);
    m_scalarBarWidget->SetEnabled(true);

    m_renderer->AddViewProp(m_colorMappingLegend);
}

void RenderView::updateWindowTitle()
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

vtkRenderWindow * RenderView::renderWindow()
{
    return m_ui->qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * RenderView::renderWindow() const
{
    return m_ui->qvtkMain->GetRenderWindow();
}

IPickingInteractorStyle * RenderView::interactorStyle()
{
    return m_interactorStyle;
}

const IPickingInteractorStyle * RenderView::interactorStyle() const
{
    return m_interactorStyle;
}

void RenderView::updateGuiForActor(vtkActor * actor)
{
    assert(actor);

    vtkInformation * inputInfo = actor->GetMapper()->GetInformation();

    QString propertyName;
    if (inputInfo->Has(Input::NameKey()))
        propertyName = Input::NameKey()->Get(inputInfo);

    m_renderConfigWidget.setRenderedData(m_actorToRenderedData[actor]);
    m_dataChooser.setMapping(windowTitle(), &m_scalarMapping);
}