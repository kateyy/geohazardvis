#include "RenderViewStrategyImage2D.h"

#include <cassert>

#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QLayout>
#include <QToolBar>

#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkCubeAxesActor.h>
#include <vtkTextProperty.h>
#include <vtkProperty.h>

#include <vtkLineWidget2.h>
#include <vtkLineRepresentation.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkPolyData.h>
#include <vtkEventQtSlotConnect.h>

#include <core/DataSetHandler.h>
#include <core/utility/vtkhelper.h>
#include <core/utility/vtkcamerahelper.h>
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


RenderViewStrategyImage2D::RenderViewStrategyImage2D(RendererImplementationBase3D & context, QObject * parent)
    : RenderViewStrategy(context, parent)
    , m_isInitialized(false)
    , m_previewRenderer(nullptr)
{
    connect(&context.renderView(), &AbstractRenderView::visualizationsChanged, 
        this, &RenderViewStrategyImage2D::checkSourceExists);
}

RenderViewStrategyImage2D::~RenderViewStrategyImage2D()
{
    if (m_previewRenderer && !m_previewProfiles.isEmpty())
    {
        m_previewRenderer->prepareDeleteData(m_previewProfiles);
        if (m_previewRenderer->visualizations().isEmpty())
            m_previewRenderer->dockWidgetParent()->close();
    }

    qDeleteAll(m_previewProfiles);
}

void RenderViewStrategyImage2D::setInputImages(const QList<ImageDataObject *> & images)
{
    bool restartProfilePlot = false;
    double point1[3], point2[3];
    if (!m_previewProfiles.isEmpty())   // currently plotting
    {
        m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1);
        m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2);

        abortProfilePlot();
        restartProfilePlot = true;
    }

    m_inputImages = images;

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
        if (dynamic_cast<ImageDataObject *>(dataObject))
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;
    }

    return compatible;
}

bool RenderViewStrategyImage2D::canApplyTo(const QList<RenderedData *> & renderedData)
{
    for (RenderedData * rendered : renderedData)
        if (!dynamic_cast<ImageDataObject *>(rendered->dataObject()))
            return false;

    return true;
}

void RenderViewStrategyImage2D::startProfilePlot()
{
    assert(m_previewProfiles.isEmpty() && !m_previewRenderer);

    // check input images

    assert(m_currentPlottingImages.isEmpty());

    // no inputs set, fetch data from our context
    if (m_inputImages.isEmpty())
    {
        ImageDataObject * image = nullptr;
        for (DataObject * dataObject : m_context.renderView().dataObjects())
        {
            image = dynamic_cast<ImageDataObject *>(dataObject);
            m_currentPlottingImages << image;
        }
    }
    // inputs explicitly set
    else
    {
        m_currentPlottingImages = m_inputImages;
    }

    // if there are no inputs, just ignore the request
    if (m_currentPlottingImages.isEmpty())
        return;

    m_profilePlotAction->setEnabled(false);


    // create profiles

    for (auto inputImage : m_currentPlottingImages)
    {
        QString name = inputImage->name() + " plot";

        m_previewProfiles << new ImageProfileData(name, inputImage);
    }

    // place the line widget
    
    m_lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
    VTK_CREATE(vtkLineRepresentation, repr);
    repr->SetLineColor(1, 0, 0);
    m_lineWidget->SetRepresentation(repr);
    m_lineWidget->SetInteractor(m_context.interactor());
    m_lineWidget->On();

    double bounds[6];

    m_context.dataBounds(bounds);

    bounds[4] += g_lineZOffset;
    bounds[5] += g_lineZOffset;

    repr->PlaceWidget(bounds);

    m_context.render();


    lineMoved();

    m_previewRenderer = DataMapping::instance().openInRenderView(m_previewProfiles);
    if (!m_previewRenderer) // in case the user closed the view again
    {
        abortProfilePlot();
        return;
    }

    m_previewRendererConnections << 
        connect(m_previewRenderer, &AbstractDataView::closed, this, &RenderViewStrategyImage2D::abortProfilePlot);

    m_vtkConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    m_vtkConnect->Connect(m_lineWidget->GetLineRepresentation()->GetLineHandleRepresentation(), vtkCommand::ModifiedEvent, this, SLOT(lineMoved()));
    m_vtkConnect->Connect(m_lineWidget->GetLineRepresentation()->GetPoint1Representation(), vtkCommand::ModifiedEvent, this, SLOT(lineMoved()));
    m_vtkConnect->Connect(m_lineWidget->GetLineRepresentation()->GetPoint2Representation(), vtkCommand::ModifiedEvent, this, SLOT(lineMoved()));

    m_profilePlotAcceptAction->setVisible(true);
    m_profilePlotAbortAction->setVisible(true);
}

void RenderViewStrategyImage2D::acceptProfilePlot()
{
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    DataSetHandler::instance().addData(m_previewProfiles);
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
        oldPreviewRenderer->dockWidgetParent()->close();

    qDeleteAll(m_previewProfiles);
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

    for (auto profile : m_previewProfiles)
        static_cast<ImageProfileData *>(profile)->setPoints(point1, point2);
}

void RenderViewStrategyImage2D::checkSourceExists()
{
    // don't check here, if the user of this class explicitly set the inputs
    if (!m_inputImages.isEmpty())
        return;

    // recreate the plot, if any of the automatically fetches objects was removed from the coutext
    for (auto img : m_currentPlottingImages)
    {
        if (m_context.renderView().dataObjects().contains(img))
            continue;

        abortProfilePlot();
        startProfilePlot();
        return;
    }
}
