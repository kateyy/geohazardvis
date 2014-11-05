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
#include <core/scalar_mapping/ScalarToColorMapping.h>

#include <gui/SelectionHandler.h>
#include <gui/data_view/RenderViewStrategyNull.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>
#include <gui/rendering_interaction/InteractorStyle3D.h>
#include <gui/rendering_interaction/InteractorStyleImage.h>
#include <gui/widgets/ScalarMappingChooser.h>
#include <gui/widgets/VectorMappingChooser.h>
#include <gui/widgets/RenderConfigWidget.h>


RenderView::RenderView(
    int index,
    QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_ui(new Ui_RenderView())
    , m_strategy(nullptr)
    , m_emptyStrategy(new RenderViewStrategyNull(*this))
    , m_axesEnabled(true)
    , m_scalarMapping(new ScalarToColorMapping())
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();
    createAxes();

    setupColorMappingLegend();

    updateTitle();

    connect(m_scalarMapping, &ScalarToColorMapping::colorLegendVisibilityChanged,
        [this] (bool visible) {
        m_scalarBarWidget->SetEnabled(visible);
    });

    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked, this, &RenderView::updateGuiForSelectedData);

    SelectionHandler::instance().addRenderView(this);
}

RenderView::~RenderView()
{
    SelectionHandler::instance().removeRenderView(this); 

    qDeleteAll(m_renderedData);
    qDeleteAll(m_renderedDataCache);

    delete m_emptyStrategy;
    delete m_scalarMapping;
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
    connect(m_interactorStyle.Get(), &PickingInteractorStyleSwitch::dataPicked, this, &RenderView::updateGuiForSelectedData);
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

void RenderView::addDataObjects(const QList<DataObject *> & uncheckedDataObjects, QList<DataObject *> & incompatibleObjects)
{
    if (uncheckedDataObjects.isEmpty())
        return;

    bool wasEmpty = m_renderedData.isEmpty();

    if (wasEmpty)
        emit resetStrategie(uncheckedDataObjects);

    RenderedData * aNewObject = nullptr;

    QList<DataObject *> dataObjects = strategy().filterCompatibleObjects(uncheckedDataObjects, incompatibleObjects);

    if (dataObjects.isEmpty())
        return;

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

        updateGuiForSelectedData(aNewObject);

        emit renderedDataChanged();
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
}

void RenderView::hideDataObjects(const QList<DataObject *> & dataObjects)
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

    emit renderedDataChanged();

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

    QList<RenderedData *> toDelete = removeFromInternalLists({ dataObject });

    updateGuiForRemovedData();

    emit renderedDataChanged();

    render();

    qDeleteAll(toDelete);
}

void RenderView::removeDataObjects(const QList<DataObject *> & dataObjects)
{
    for (DataObject * dataObject : dataObjects)
        removeDataObject(dataObject);

    if (m_renderedData.isEmpty())
        setStrategy(nullptr);
}

QList<RenderedData *> RenderView::removeFromInternalLists(QList<DataObject *> dataObjects)
{
    QList<RenderedData *> toDelete;
    for (DataObject * dataObject : dataObjects)
    {
        RenderedData * rendered = m_dataObjectToRendered.value(dataObject, nullptr);
        assert(rendered);

        m_dataObjectToRendered.remove(dataObject);
        assert(m_renderedData.count(rendered) + m_renderedDataCache.count(rendered) == 1);
        if (!m_renderedData.removeOne(rendered))
            m_renderedDataCache.removeOne(rendered);

        toDelete << rendered;
    }

    return toDelete;
}

QList<DataObject *> RenderView::dataObjects() const
{
    QList<DataObject *> objs;
    for (RenderedData * r : m_renderedData)
        objs << r->dataObject();

    return objs;
}

const QList<RenderedData *> & RenderView::renderedData() const
{
    return m_renderedData;
}

DataObject * RenderView::highlightedData() const
{
    DataObject * highlighted = m_interactorStyle->highlightedObject();

    if (!highlighted && !m_renderedData.isEmpty())
        highlighted = m_renderedData.first()->dataObject();

    return highlighted;
}

RenderedData * RenderView::highlightedRenderedData() const
{
    return m_dataObjectToRendered.value(highlightedData());
}

ScalarToColorMapping * RenderView::scalarMapping()
{
    return m_scalarMapping;
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

    m_axesActor->SetBounds(m_dataBounds);
}

void RenderView::createAxes()
{
    m_axesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    m_axesActor->SetCamera(m_renderer->GetActiveCamera());
    m_axesActor->SetFlyModeToOuterEdges();
    m_axesActor->SetGridLineLocation(VTK_GRID_LINES_FURTHEST);
    //m_axesActor->SetUseTextActor3D(true);
    m_axesActor->SetTickLocationToBoth();
    // fix strange rotation of z-labels
    m_axesActor->GetLabelTextProperty(2)->SetOrientation(90);

    double axesColor[3] = { 0, 0, 0 };
    double gridColor[3] = { 0.7, 0.7, 0.7 };

    m_axesActor->GetXAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetYAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetZAxesLinesProperty()->SetColor(axesColor);
    m_axesActor->GetXAxesGridlinesProperty()->SetColor(gridColor);
    m_axesActor->GetYAxesGridlinesProperty()->SetColor(gridColor);
    m_axesActor->GetZAxesGridlinesProperty()->SetColor(gridColor);

    for (int i = 0; i < 3; ++i) {
        m_axesActor->GetTitleTextProperty(i)->SetColor(axesColor);
        m_axesActor->GetLabelTextProperty(i)->SetColor(axesColor);
    }

    m_axesActor->XAxisMinorTickVisibilityOff();
    m_axesActor->YAxisMinorTickVisibilityOff();
    m_axesActor->ZAxisMinorTickVisibilityOff();

    m_axesActor->DrawXGridlinesOn();
    m_axesActor->DrawYGridlinesOn();
    m_axesActor->DrawZGridlinesOn();

    m_renderer->AddViewProp(m_axesActor);
}

void RenderView::setupColorMappingLegend()
{
    m_colorMappingLegend = m_scalarMapping->colorMappingLegend();
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

void RenderView::updateGuiForContent()
{
    RenderedData * focus = m_dataObjectToRendered.value(m_interactorStyle->highlightedObject());
    if (!focus)
        focus = m_renderedData.value(0, nullptr);

    updateGuiForSelectedData(focus);
}

void RenderView::updateGuiForSelectedData(RenderedData * renderedData)
{
    m_interactorStyle->setRenderedData(m_renderedData);

    m_scalarMapping->setRenderedData(m_renderedData);

    DataObject * current = renderedData ? renderedData->dataObject() : nullptr;

    updateTitle();

    emit selectedDataChanged(this, current);
}

void RenderView::updateGuiForRemovedData()
{
    RenderedData * nextSelection = m_renderedData.isEmpty()
        ? nullptr : m_renderedData.first();

    updateAxes();
    m_renderer->ResetCamera();

    updateGuiForSelectedData(nextSelection);
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
