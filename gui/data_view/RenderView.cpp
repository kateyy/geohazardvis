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

#include <gui/SelectionHandler.h>
#include <gui/data_view/RenderViewStrategyNull.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>
#include <gui/rendering_interaction/InteractorStyleImage.h>
#include <gui/widgets/ScalarMappingChooser.h>
#include <gui/widgets/VectorMappingChooser.h>
#include <gui/widgets/RenderConfigWidget.h>


using namespace std;


RenderView::RenderView(
    int index,
    ScalarMappingChooser & scalarMappingChooser,
    VectorMappingChooser & vectorMappingChooser,
    RenderConfigWidget & renderConfigWidget,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_ui(new Ui_RenderView())
    , m_strategy(nullptr)
    , m_emptyStrategy(new RenderViewStrategyNull(*this))
    , m_axesEnabled(true)
    , m_scalarMappingChooser(scalarMappingChooser)
    , m_vectorMappingChooser(vectorMappingChooser)
    , m_renderConfigWidget(renderConfigWidget)
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();
    m_axesActor = createAxes(*m_renderer);
    setupColorMappingLegend();

    updateTitle();

    connect(&m_scalarMappingChooser, &ScalarMappingChooser::renderSetupChanged, this, &RenderView::render);
    connect(&m_vectorMappingChooser, &VectorMappingChooser::renderSetupChanged, this, &RenderView::render);
    connect(&m_scalarMapping, &ScalarToColorMapping::colorLegendVisibilityChanged,
        [this] (bool visible) {
        m_scalarBarWidget->SetEnabled(visible);
    });

    SelectionHandler::instance().addRenderView(this);
}

RenderView::~RenderView()
{
    disconnect(&m_scalarMappingChooser, &ScalarMappingChooser::renderSetupChanged, this, &RenderView::render);
    disconnect(&m_vectorMappingChooser, &VectorMappingChooser::renderSetupChanged, this, &RenderView::render);

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
    qDeleteAll(m_renderedDataCache);

    delete m_emptyStrategy;
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

void RenderView::highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId)
{
    interactorStyle()->highlightCell(dataObject, itemId);
}

void RenderView::setupRenderer()
{
    m_ui->qvtkMain->GetRenderWindow()->SetAAFrames(0);
    //m_ui->qvtkMain->GetRenderWindow()->SetMultiSamples(0);

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

    m_interactorStyle->addStyle("InteractorStyle3D", vtkSmartPointer<InteractorStyle3D>::New());
    m_interactorStyle->addStyle("InteractorStyleImage", vtkSmartPointer<InteractorStyleImage>::New());

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::pointInfoSent, this, &RenderView::ShowInfo);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked, this, &RenderView::updateGuiForData);
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::cellPicked, this, &AbstractDataView::objectPicked);

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

void RenderView::setStrategy(RenderViewStrategy * strategy)
{
    if (m_strategy)
        m_strategy->deactivate();

    m_strategy = strategy;
    
    if (m_strategy)
        m_strategy->activate();
}

RenderedData * RenderView::addDataObject(DataObject * dataObject)
{
    updateTitle(dataObject->name() + " (loading to GPU)");
    QApplication::processEvents();

    assert(dataObject->is3D() == contains3dData());

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

    bool wasEmpty = m_renderedData.isEmpty();

    if (wasEmpty)
        emit resetStrategie(dataObjects);

    RenderedData * aNewObject = nullptr;

    QStringList incompatibleObjects = strategy().checkCompatibleObjects(dataObjects);

    if (dataObjects.isEmpty())
    {
        warnIncompatibleObjects(incompatibleObjects);
        return;
    }

    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * cachedRendered = m_dataObjectToRendered.value(dataObject);

        // create new rendered representation
        if (!cachedRendered)
        {
            aNewObject = addDataObject(dataObject);
            connect(dataObject, &DataObject::boundsChanged, this, &RenderView::updateAxes);

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

    if (aNewObject)
    {
        updateAxes();

        updateTitle();

        updateGuiForData(aNewObject);

        emit renderedDataChanged(m_renderedData);
    }

    if (wasEmpty)
    {
        strategy().resetCamera(*m_renderer->GetActiveCamera());
    }

    if (aNewObject)
    {
        m_renderer->ResetCamera();

        render();
    }


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

        disconnect(dataObject, &DataObject::boundsChanged, this, &RenderView::updateAxes);
    }

    if (!changed)
        return;

    updateGuiForRemovedData();

    emit renderedDataChanged(m_renderedData);

    if (m_renderedData.isEmpty())
        setStrategy(nullptr);

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

    if (m_renderedData.isEmpty())
        setStrategy(nullptr);
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

const double * RenderView::getDataBounds() const
{
    return m_dataBounds;
}

void RenderView::getDataBounds(double bounds[6]) const
{
    for (int i = 0; i < 6; ++i)
        bounds[i] = m_dataBounds[i];
}

RenderViewStrategy & RenderView::strategy() const
{
    assert(m_emptyStrategy);
    if (m_strategy)
        return *m_strategy;

    return *m_emptyStrategy;
}

void RenderView::updateAxes()
{
    vtkBoundingBox bounds;

    for (RenderedData * renderedData : m_renderedData)
    {
        assert(renderedData->isVisible());
        bounds.AddBounds(renderedData->dataObject()->bounds());
    }

    bounds.GetBounds(m_dataBounds);

    // hide axes if we don't have visible objects
    if (!bounds.IsValid())
    {
        m_axesActor->VisibilityOff();
        return;
    }
    
    m_axesActor->SetVisibility(m_axesEnabled);

    m_renderer->AddViewProp(m_axesActor);

    m_axesActor->SetBounds(m_dataBounds);
    m_axesActor->SetRebuildAxes(true);
}

vtkSmartPointer<vtkCubeAxesActor> RenderView::createAxes(vtkRenderer & renderer)
{
    VTK_CREATE(vtkCubeAxesActor, cubeAxes);
    cubeAxes->SetCamera(renderer.GetActiveCamera());
    cubeAxes->SetFlyModeToOuterEdges();
    cubeAxes->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);
    //cubeAxes->SetUseTextActor3D(true);
    cubeAxes->SetTickLocationToBoth();
    // fix strange rotation of z-labels
    cubeAxes->GetLabelTextProperty(2)->SetOrientation(90);

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

    cubeAxes->XAxisMinorTickVisibilityOff();
    cubeAxes->YAxisMinorTickVisibilityOff();
    cubeAxes->ZAxisMinorTickVisibilityOff();

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
    m_scalarBarWidget->EnabledOff();

    m_renderer->AddViewProp(m_colorMappingLegend);
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

PickingInteractorStyleSwitch * RenderView::interactorStyleSwitch()
{
    return m_interactorStyle;
}

vtkLightKit * RenderView::lightKit()
{
    return m_lightKit;
}

vtkScalarBarWidget * RenderView::colorLegendWidget()
{
    return m_scalarBarWidget;
}

vtkCubeAxesActor * RenderView::axesActor()
{
    return m_axesActor;
}

void RenderView::setEnableAxes(bool enabled)
{
    if (m_axesEnabled == enabled)
        return;

    m_axesEnabled = enabled;

    m_axesActor->SetVisibility(m_axesEnabled && !m_renderedData.isEmpty());

    render();
}

bool RenderView::axesEnabled() const
{
    return m_axesEnabled;
}

bool RenderView::contains3dData() const
{
    return strategy().contains3dData();
}

void RenderView::updateGuiForData(RenderedData * renderedData)
{
    m_interactorStyle->setRenderedData(m_renderedData);
    m_renderConfigWidget.setRenderedData(index(), renderedData);
    m_scalarMapping.setRenderedData(m_renderedData);
    m_scalarMappingChooser.setMapping(this,  &m_scalarMapping);
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
    m_scalarMappingChooser.setMapping(this, &m_scalarMapping);

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
