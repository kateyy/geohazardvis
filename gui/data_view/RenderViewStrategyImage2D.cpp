#include "RenderViewStrategyImage2D.h"

#include <cassert>
#include <memory>

#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QLayout>
#include <QSet>
#include <QToolBar>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkCubeAxesActor.h>
#include <vtkLineRepresentation.h>
#include <vtkLineWidget2.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkTextProperty.h>

#include <core/DataSetHandler.h>
#include <core/utility/vtkcamerahelper.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>

const bool RenderViewStrategyImage2D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategyImage2D>();

namespace
{
    // ensures that the line is placed in front of other contents
    const float g_lineZOffset = 0.00001f;
}


RenderViewStrategyImage2D::RenderViewStrategyImage2D(RendererImplementationBase3D & context)
    : RenderViewStrategy(context)
    , m_isInitialized(false)
    , m_previewRenderer(nullptr)
{
    connect(&context.renderView(), &AbstractRenderView::visualizationsChanged, 
        this, &RenderViewStrategyImage2D::checkSourceExists);
}

RenderViewStrategyImage2D::~RenderViewStrategyImage2D()
{
    if (m_previewRenderer && !m_previewProfiles.empty())
    {
        QList<DataObject *> objects;
        for (auto && obj : m_previewProfiles)
            objects << obj.get();
        m_previewRenderer->prepareDeleteData(objects);
        if (m_previewRenderer->visualizations().isEmpty())
            m_previewRenderer->close();
    }
}

void RenderViewStrategyImage2D::setInputData(const QList<DataObject *> & inputData)
{
    if (m_inputData.toSet() == inputData.toSet())
        return;

    bool restartProfilePlot = false;
    double point1[3], point2[3];
    if (!m_previewProfiles.empty())   // currently plotting
    {
        m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1);
        m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2);

        abortProfilePlot();
        restartProfilePlot = true;
    }

    m_inputData = inputData;

    if (restartProfilePlot)
    {
        startProfilePlot();

        m_lineWidget->GetLineRepresentation()->SetPoint1WorldPosition(point1);
        m_lineWidget->GetLineRepresentation()->SetPoint2WorldPosition(point2);
    }
}

void RenderViewStrategyImage2D::initialize()
{
    if (m_isInitialized)
        return;

    m_profilePlotAction = new QAction(QIcon(":/icons/graph_line"), "create &profile plot", nullptr);
    connect(m_profilePlotAction, &QAction::triggered, this, &RenderViewStrategyImage2D::startProfilePlot);
    m_profilePlotAcceptAction = new QAction(QIcon(":/icons/yes"), "", nullptr);
    connect(m_profilePlotAcceptAction, &QAction::triggered, this, &RenderViewStrategyImage2D::acceptProfilePlot);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction = new QAction(QIcon(":/icons/no"), "", nullptr);
    connect(m_profilePlotAbortAction, &QAction::triggered, this, &RenderViewStrategyImage2D::abortProfilePlot);
    m_profilePlotAbortAction->setVisible(false);

    m_actions << m_profilePlotAction << m_profilePlotAcceptAction << m_profilePlotAbortAction;

    m_isInitialized = true;
}

QString RenderViewStrategyImage2D::name() const
{
    return "2D image";
}

void RenderViewStrategyImage2D::activate()
{
    initialize();

    m_context.interactorStyleSwitch()->setCurrentStyle("InteractorStyleImage");

    m_context.renderView().toolBar()->addActions(m_actions);
    m_context.renderView().setToolBarVisible(true);

    m_context.axesActor()->GetLabelTextProperty(0)->SetOrientation(0);
    m_context.axesActor()->GetLabelTextProperty(1)->SetOrientation(90);
    m_context.axesActor()->SetUse2DMode(true);
}

void RenderViewStrategyImage2D::deactivate()
{
    for (auto action : m_actions)
    {
        m_context.renderView().toolBar()->removeAction(action);
        action->setParent(nullptr);
    }
    m_context.renderView().setToolBarVisible(false);

    m_context.axesActor()->SetUse2DMode(false);
}

bool RenderViewStrategyImage2D::contains3dData() const
{
    return false;
}

void RenderViewStrategyImage2D::resetCamera(vtkCamera & camera)
{
    camera.SetViewUp(0, 1, 0);
    camera.SetFocalPoint(0, 0, 0);
    camera.SetPosition(0, 0, 1);
}

QList<DataObject *> RenderViewStrategyImage2D::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const
{
    QList<DataObject *> compatible;

    for (DataObject * dataObject : dataObjects)
    {
        // TODO limit the automatic strategy switch to 2D image data, until there is a gui switch for 2D/3D interaction.

        if (dynamic_cast<ImageDataObject *>(dataObject))
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;

        /*std::unique_ptr<RenderedData> rendered{ dataObject->createRendered() };
        if (rendered)
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;*/
    }

    return compatible;
}

bool RenderViewStrategyImage2D::canApplyTo(const QList<RenderedData *> & renderedData)
{
    // TODO limit the automatic strategy switch to 2D image data, until there is a gui switch for 2D/3D interaction.
    for (RenderedData * rendered : renderedData)
        if (!dynamic_cast<ImageDataObject *>(&rendered->dataObject()))
            return false;

    // just exclude context data (the profile plots etc), but they don't apply heres
    return true;
}

void RenderViewStrategyImage2D::startProfilePlot()
{
    assert(m_previewProfiles.empty() && !m_previewRenderer);

    // check input images

    assert(m_currentPlottingImages.isEmpty());

    // no inputs set, fetch data from our context
    if (m_inputData.isEmpty())
    {
        m_currentPlottingImages = m_context.renderView().dataObjects();
    }
    // inputs explicitly set
    else
    {
        m_currentPlottingImages = m_inputData;
    }

    // if there are no inputs, just ignore the request
    if (m_currentPlottingImages.isEmpty())
        return;

    m_profilePlotAction->setEnabled(false);


    // for each sub-view, check which combinations of active scalars and input images we have to plot
    for (unsigned int i = 0; i < m_context.renderView().numberOfSubViews(); ++i)
    {
        auto & scalars = m_context.colorMapping(i)->currentScalarsName();

        for (auto inputImage : m_currentPlottingImages)
        {
            if (!m_context.renderView().contains(inputImage, i))
                continue;

            auto profile = std::make_unique<ImageProfileData>(
                inputImage->name() + " plot",
                *inputImage,
                scalars);

            if (!profile->isValid())
                continue;

            m_previewProfiles.push_back(std::move(profile));
        }
    }

    if (m_previewProfiles.empty())
    {
        abortProfilePlot();
        return;
    }

    // place the line widget
    
    m_lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
    auto repr = vtkSmartPointer<vtkLineRepresentation>::New();
    repr->SetLineColor(1, 0, 0);
    m_lineWidget->SetRepresentation(repr);
    m_lineWidget->SetInteractor(m_context.interactor());
    m_lineWidget->SetCurrentRenderer(m_context.renderer(0u));   // put the widget in the first renderer, for now
    m_lineWidget->On();

    double bounds[6];

    m_context.dataBounds(bounds);

    bounds[4] += g_lineZOffset;
    bounds[5] += g_lineZOffset;

    repr->PlaceWidget(bounds);

    m_context.render();


    lineMoved();

    QList<DataObject *> profiles;
    for (auto && profile : m_previewProfiles)
        profiles << profile.get();
    m_previewRenderer = DataMapping::instance().openInRenderView(profiles);
    if (!m_previewRenderer) // in case the user closed the view again
    {
        abortProfilePlot();
        return;
    }

    m_previewRendererConnections << 
        connect(m_previewRenderer, &AbstractDataView::closed, this, &RenderViewStrategyImage2D::abortProfilePlot);

    for (auto it = m_observerTags.begin(); it != m_observerTags.end(); ++it)
    {
        it.key()->RemoveObserver(it.value());
    }
    m_observerTags.clear();

    auto addLineObservation = [this] (vtkObject * subject)
    {
        auto tag = subject->AddObserver(vtkCommand::ModifiedEvent, this, &RenderViewStrategyImage2D::lineMoved);
        m_observerTags.insert(subject, tag);
    };

    addLineObservation(m_lineWidget->GetLineRepresentation()->GetLineHandleRepresentation());
    addLineObservation(m_lineWidget->GetLineRepresentation()->GetPoint1Representation());
    addLineObservation(m_lineWidget->GetLineRepresentation()->GetPoint2Representation());

    m_profilePlotAcceptAction->setVisible(true);
    m_profilePlotAbortAction->setVisible(true);
}

void RenderViewStrategyImage2D::acceptProfilePlot()
{
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    DataSetHandler::instance().takeData(std::move(m_previewProfiles));
    m_previewProfiles.clear();
    for (auto & c : m_previewRendererConnections)
        disconnect(c);
    m_previewRendererConnections.clear();

    m_previewRenderer = nullptr;
    m_currentPlottingImages.clear();

    m_lineWidget = nullptr;
    m_context.render();

    m_profilePlotAction->setEnabled(true);
}

void RenderViewStrategyImage2D::abortProfilePlot()
{
    for (auto & c : m_previewRendererConnections)
        disconnect(c);
    m_previewRendererConnections.clear();

    auto oldPreviewRenderer = m_previewRenderer;
    m_previewRenderer = nullptr;
    m_currentPlottingImages.clear();

    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    m_lineWidget = nullptr;
    m_context.render();

    // this becomes nullptr in case the user closed the viewer while we were starting a plot
    if (oldPreviewRenderer)
        oldPreviewRenderer->close();

    m_previewProfiles.clear();

    m_profilePlotAction->setEnabled(true);
}

void RenderViewStrategyImage2D::lineMoved()
{
    assert(m_previewProfiles.size() > 0 && m_lineWidget);

    double point1[3], point2[3];
    m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1);
    m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2);
    point1[2] -= g_lineZOffset;
    point2[2] -= g_lineZOffset;

    for (auto && profile : m_previewProfiles)
        static_cast<ImageProfileData *>(profile.get())->setPoints(point1, point2);
}

void RenderViewStrategyImage2D::checkSourceExists()
{
    // don't check here, if the user of this class explicitly set the inputs
    if (!m_inputData.isEmpty())
        return;

    // recreate the plot, if any of the automatically fetches objects was removed from the context
    for (auto img : m_currentPlottingImages)
    {
        if (m_context.renderView().dataObjects().contains(img))
            continue;

        abortProfilePlot();
        startProfilePlot();
        return;
    }
}
