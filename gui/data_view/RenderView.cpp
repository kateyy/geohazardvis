#include "RenderView.h"
#include "ui_RenderView.h"

#include <cassert>

#include <QMessageBox>

#include <vtkBoundingBox.h>

#include <vtkCubeAxesActor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkProperty2D.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkLightCollection.h>
#include <vtkLightKit.h>

#include <core/vtkhelper.h>
#include <core/vtkcamerahelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>

#include "SelectionHandler.h"
#include "rendering_interaction/PickingInteractorStyleSwitch.h"
#include "rendering_interaction/InteractorStyle3D.h"
#include "rendering_interaction/InteractorStyleImage.h"
#include "widgets/ScalarMappingChooser.h"
#include "widgets/VectorMappingChooser.h"
#include "widgets/RenderConfigWidget.h"


using namespace std;


RenderView::RenderView(
    int index,
    ScalarMappingChooser & scalarMappingChooser,
    VectorMappingChooser & vectorMappingChooser,
    RenderConfigWidget & renderConfigWidget,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_ui(new Ui_RenderView())
    , m_scalarMappingChooser(scalarMappingChooser)
    , m_vectorMappingChooser(vectorMappingChooser)
    , m_renderConfigWidget(renderConfigWidget)
    , m_contains3DData(true)
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();
    setupColorMappingLegend();

    updateTitle();

    connect(&m_scalarMappingChooser, &ScalarMappingChooser::renderSetupChanged, this, &RenderView::render);
    connect(&m_vectorMappingChooser, &VectorMappingChooser::renderSetupChanged, this, &RenderView::render);

    SelectionHandler::instance().addRenderView(this);
}

RenderView::~RenderView()
{
    SelectionHandler::instance().removeRenderView(this);

    m_renderConfigWidget.clear();

    if (m_scalarMappingChooser.mapping() == &m_scalarMapping)
        m_scalarMappingChooser.setMapping();

    for (RenderedData * renderedData : m_renderedData)
    {
        if (renderedData->vectorMapping() == m_vectorMappingChooser.mapping())
        {
            m_vectorMappingChooser.setMapping();
            break;
        }
    }   

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

QString RenderView::friendlyName() const
{
    QString name;
    for (RenderedData * renderedData : m_renderedData)
        name += ", " + renderedData->dataObject()->name();

    if (name.isEmpty())
        name = "(empty)";
    else
        name.remove(0, 2);

    name = QString::number(index()) + ": " + name;

    return name;
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
    
    m_renderer->GetLights()->RemoveAllItems();
    m_lightKit = vtkSmartPointer<vtkLightKit>::New();
    m_lightKit->AddLightsToRenderer(m_renderer);
}

void RenderView::setupInteraction()
{
    m_interactorStyle = vtkSmartPointer<PickingInteractorStyleSwitch>::New();
    m_interactorStyle->SetDefaultRenderer(m_renderer);

    m_interactorStyle->addStyle("InteractorStyle3D", InteractorStyle3D::New());
    m_interactorStyle->addStyle("InteractorStyleImage", InteractorStyleImage::New());

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::pointInfoSent, this, &RenderView::ShowInfo);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked, this, &RenderView::updateGuiForData);

    m_ui->qvtkMain->GetRenderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);
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
    updateTitle(dataObject->name() + " (loading to GPU)");
    QApplication::processEvents();

    assert(dataObject->is3D() == m_contains3DData);

    RenderedData * renderedData = dataObject->createRendered();
    if (!renderedData)
        return nullptr;

    m_attributeActors << renderedData->attributeActors();
    for (vtkActor * actor : renderedData->actors())
        m_renderer->AddViewProp(actor);

    connect(renderedData, &RenderedData::attributeActorsChanged, this, &RenderView::fetchAllAttributeActors);

    m_renderedData << renderedData;

    connect(renderedData, &RenderedData::geometryChanged, this, &RenderView::render);

    m_dataObjectToRendered.insert(dataObject, renderedData);

    return renderedData;
}

void RenderView::addDataObjects(QList<DataObject *> dataObjects)
{
    if (dataObjects.isEmpty())
        return;

    RenderedData * aNewObject = nullptr;

    QStringList incompatibleObjects = checkCompatibleObjects(dataObjects);

    if (dataObjects.isEmpty())
    {
        warnIncompatibleObjects(incompatibleObjects);
        return;
    }

    bool wasEmpty = m_renderedData.isEmpty();

    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * cachedRendered = m_dataObjectToRendered.value(dataObject);

        // create new rendered representation
        if (!cachedRendered)
        {
            aNewObject = addDataObject(dataObject);
            continue;
        }

        aNewObject = cachedRendered;

        // reuse cached data
        if (m_renderedData.contains(cachedRendered))
        {
            assert(false);
            continue;
        }

        assert(m_renderedDataCache.count(cachedRendered) == 1);
        m_renderedDataCache.removeOne(cachedRendered);
        m_renderedData << cachedRendered;
        cachedRendered->setVisible(true);
    }

    vtkCamera * camera = m_renderer->GetActiveCamera();
    if (wasEmpty)
    {
        if (m_contains3DData)
        {
            camera->SetViewUp(0, 0, 1);
            TerrainCamera::setAzimuth(camera, 0);
            TerrainCamera::setVerticalElevation(camera, 45);
        }
        else
        {
            camera->SetViewUp(0, 1, 0);
            camera->SetFocalPoint(0, 0, 0);
            camera->SetPosition(0, 0, 1);
        }
    }

    if (aNewObject)
    {
        updateAxes();
        m_renderer->ResetCamera();

        updateTitle();

        updateGuiForData(aNewObject);
    }

    render();

    if (!incompatibleObjects.isEmpty())
        warnIncompatibleObjects(incompatibleObjects);
}

void RenderView::hideDataObjects(QList<DataObject *> dataObjects)
{
    bool changed = false;
    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * rendered = m_dataObjectToRendered.value(dataObject);
        if (!rendered)
            continue;

        // move data to cache if it isn't already invisible
        if (m_renderedData.removeOne(rendered))
        {
            rendered->setVisible(false);
            m_renderedDataCache << rendered;

            changed = true;
        }
        assert(!m_renderedData.contains(rendered));
        assert(m_renderedDataCache.count(rendered) == 1);
    }

    if (!changed)
        return;

    updateGuiForRemovedData();

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
    RenderedData * renderedData = m_dataObjectToRendered.value(dataObject, nullptr);

    // we didn't render this object
    if (!renderedData)
        return;

    for (vtkActor * actor : renderedData->actors())
        m_renderer->RemoveViewProp(actor);

    removeFromInternalLists({ dataObject });

    updateGuiForRemovedData();
}

void RenderView::removeDataObjects(QList<DataObject *> dataObjects)
{
    // TODO optimize as needed
    for (DataObject * dataObject : dataObjects)
        removeDataObject(dataObject);
}

QStringList RenderView::checkCompatibleObjects(QList<DataObject *> & dataObjects)
{
    assert(!dataObjects.isEmpty());

    // allow data type switch if nothing is visible
    if (m_renderedData.isEmpty())
    {
        // using the first new object as reference for our new type
        m_contains3DData = dataObjects.first()->is3D();
    }
    updateInteractionType();

    QStringList invalidObjects;
    QList<DataObject *> compatibleObjects;

    for (DataObject * dataObject : dataObjects)
    {
        if (dataObject->is3D() == m_contains3DData)
            compatibleObjects << dataObject;
        else
            invalidObjects << dataObject->name();
    }

    dataObjects = compatibleObjects;

    return invalidObjects;
}

void RenderView::clearInternalLists()
{
    qDeleteAll(m_renderedData);

    m_renderedData.clear();
    m_dataObjectToRendered.clear();
}

void RenderView::removeFromInternalLists(QList<DataObject *> dataObjects)
{
    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * rendered = m_dataObjectToRendered.value(dataObject, nullptr);
        assert(rendered);

        m_dataObjectToRendered.remove(dataObject);
        assert(m_renderedData.count(rendered) + m_renderedDataCache.count(rendered) == 1);
        if (!m_renderedData.removeOne(rendered))
            m_renderedDataCache.removeOne(rendered);

        delete rendered;
    }
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

void RenderView::updateAxes()
{
    vtkBoundingBox bounds;

    for (RenderedData * renderedData : m_renderedData)
    {
        assert(renderedData->isVisible());
        bounds.AddBounds(renderedData->dataObject()->bounds());
    }

    // hide axes if we don't have visible objects
    if (!bounds.IsValid() && m_axesActor)
    {
        m_axesActor->SetVisibility(false);
        m_scalarBarWidget->SetEnabled(false);
        return;
    }

    if (!m_axesActor)
        m_axesActor = createAxes(*m_renderer);
    
    m_axesActor->SetVisibility(true);
    m_scalarBarWidget->SetEnabled(true);

    m_renderer->AddViewProp(m_axesActor);

    double rawBounds[6];
    bounds.GetBounds(rawBounds);
    m_axesActor->SetBounds(rawBounds);
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
    m_scalarBarWidget->SetInteractor(renderWindow()->GetInteractor());
    m_scalarBarWidget->SetEnabled(true);

    m_renderer->AddViewProp(m_colorMappingLegend);
}

void RenderView::updateInteractionType()
{
    if (m_contains3DData)
        setInteractorStyle("InteractorStyle3D");
    else
        setInteractorStyle("InteractorStyleImage");
}

void RenderView::warnIncompatibleObjects(QStringList incompatibleObjects)
{
    QMessageBox::warning(this, "Invalid data selection", QString("Cannot render 2D and 3D data in the same render view!")
        + QString("\nDiscarded objects:\n") + incompatibleObjects.join('\n'));
}

vtkRenderer * RenderView::renderer()
{
    return m_renderer;
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

vtkLightKit * RenderView::lightKit()
{
    return m_lightKit;
}

bool RenderView::contains3dData() const
{
    return m_contains3DData;
}

void RenderView::updateGuiForData(RenderedData * renderedData)
{
    m_interactorStyle->setRenderedData(m_renderedData);
    m_renderConfigWidget.setRenderedData(index(), renderedData);
    m_scalarMapping.setRenderedData(m_renderedData);
    m_scalarMappingChooser.setMapping(friendlyName(), &m_scalarMapping);
    m_vectorMappingChooser.setMapping(index(), renderedData->vectorMapping());
}

void RenderView::updateGuiForRemovedData()
{
    RenderedData * nextSelection = m_renderedData.isEmpty()
        ? nullptr : m_renderedData.first();

    updateAxes();
    m_renderer->ResetCamera();

    updateTitle();

    m_interactorStyle->setRenderedData(m_renderedData);

    if (m_renderConfigWidget.rendererId() == index())
        if (!m_renderedData.contains(m_renderConfigWidget.renderedData()))
            m_renderConfigWidget.setRenderedData(index(), nextSelection);

    m_scalarMapping.setRenderedData(m_renderedData);
    m_scalarMappingChooser.setMapping(friendlyName(), &m_scalarMapping);

    VectorsToSurfaceMapping * nextMapping = nextSelection ?
        nextSelection->vectorMapping() : nullptr;
    if (m_vectorMappingChooser.rendererId() == index())
        m_vectorMappingChooser.setMapping(index(), nextMapping);
}

void RenderView::fetchAllAttributeActors()
{
    for (vtkActor * actor : m_attributeActors)
        m_renderer->RemoveViewProp(actor);

    m_attributeActors.clear();

    for (RenderedData * renderedData : m_renderedData)
        m_attributeActors << renderedData->attributeActors();

    for (RenderedData * renderedData : m_renderedDataCache)
        m_attributeActors << renderedData->attributeActors();

    for (vtkActor * actor : m_attributeActors)
        m_renderer->AddViewProp(actor);

    render();
}
