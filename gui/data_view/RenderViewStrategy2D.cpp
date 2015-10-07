#include "RenderViewStrategy2D.h"

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
#include <vtkLineRepresentation.h>
#include <vtkLineWidget2.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkTextProperty.h>

#include <core/DataSetHandler.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/rendering_interaction/CameraInteractorStyleSwitch.h>


const bool RenderViewStrategy2D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategy2D>();

namespace
{
    // ensures that the line is placed in front of other contents
    const float g_lineZOffset = 0.00001f;
}


RenderViewStrategy2D::RenderViewStrategy2D(RendererImplementationBase3D & context)
    : RenderViewStrategy(context)
    , m_isInitialized(false)
    , m_profilePlotAction(nullptr)
    , m_profilePlotAcceptAction(nullptr)
    , m_profilePlotAbortAction(nullptr)
    , m_previewRenderer(nullptr)
{
}

RenderViewStrategy2D::~RenderViewStrategy2D()
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

void RenderViewStrategy2D::setInputData(const QList<DataObject *> & inputData)
{
    if (m_inputData.toSet() == inputData.toSet())
        return;

    // reuse an existing preview renderer
    if (m_previewRenderer)
    {
        assert(!m_previewProfiles.empty());

        if (inputData.isEmpty())    // nothing to plot
        {
            abortProfilePlot();
        }
        else
        {
            // (re)create all plots later, clear the preview renderer for now
            clearProfilePlots();
        }
    }

    m_inputData = inputData;

    // start plot only if we were plotting before (setInputData is not meant to start a plot
    if (m_previewRenderer && !m_inputData.isEmpty())
    {
        startProfilePlot();
    }
}

void RenderViewStrategy2D::initialize()
{
    if (m_isInitialized)
        return;

    m_profilePlotAction = new QAction(QIcon(":/icons/graph_line"), "create &profile plot", nullptr);
    connect(m_profilePlotAction, &QAction::triggered, this, &RenderViewStrategy2D::startProfilePlot);
    m_profilePlotAcceptAction = new QAction(QIcon(":/icons/yes"), "", nullptr);
    connect(m_profilePlotAcceptAction, &QAction::triggered, this, &RenderViewStrategy2D::acceptProfilePlot);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction = new QAction(QIcon(":/icons/no"), "", nullptr);
    connect(m_profilePlotAbortAction, &QAction::triggered, this, &RenderViewStrategy2D::abortProfilePlot);
    m_profilePlotAbortAction->setVisible(false);

    m_actions << m_profilePlotAction << m_profilePlotAcceptAction << m_profilePlotAbortAction;

    m_isInitialized = true;
}

QString RenderViewStrategy2D::name() const
{
    return "2D image";
}

void RenderViewStrategy2D::onActivateEvent()
{
    initialize();

    m_context.renderView().toolBar()->addActions(m_actions);
    m_context.renderView().setToolBarVisible(true);

    updateAutomaticPlots();

    if (m_lineWidget)
    {
        m_lineWidget->On();
    }

    connect(&m_context.renderView(), &AbstractRenderView::visualizationsChanged,
        this, &RenderViewStrategy2D::updateAutomaticPlots);
}

void RenderViewStrategy2D::onDeactivateEvent()
{
    disconnect(&m_context.renderView(), &AbstractRenderView::visualizationsChanged,
        this, &RenderViewStrategy2D::updateAutomaticPlots);

    for (auto action : m_actions)
    {
        m_context.renderView().toolBar()->removeAction(action);
        action->setParent(nullptr);
    }
    m_context.renderView().setToolBarVisible(false);

    if (m_lineWidget)
    {
        // if currently plotting: disable interactivity with the line widget
        m_lineWidget->Off();
    }
}

bool RenderViewStrategy2D::contains3dData() const
{
    return false;
}

void RenderViewStrategy2D::resetCamera(vtkCamera & camera)
{
    m_context.interactorStyleSwitch()->resetCamera(camera);
}

QList<DataObject *> RenderViewStrategy2D::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const
{
    QList<DataObject *> compatible;

    for (DataObject * dataObject : dataObjects)
    {
        auto rendered = dataObject->createRendered();
        if (rendered)
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;
    }

    return compatible;
}

void RenderViewStrategy2D::startProfilePlot()
{
    assert(m_previewProfiles.empty());

    // check input images

    assert(m_activeInputData.isEmpty());

    // no inputs set, fetch data from our context
    if (m_inputData.isEmpty())
    {
        m_activeInputData = m_context.renderView().dataObjects();
    }
    // inputs explicitly set
    else
    {
        m_activeInputData = m_inputData;
    }

    // if there are no inputs, just ignore the request
    if (m_activeInputData.isEmpty())
        return;

    m_profilePlotAction->setEnabled(false);


    // for each sub-view, check which combinations of active scalars and input images we have to plot
    QSet<QPair<DataObject *, QString>> createdPlots;
    for (unsigned int i = 0; i < m_context.renderView().numberOfSubViews(); ++i)
    {
        const auto & colorMapping = m_context.colorMapping(i);
        auto & scalars = colorMapping->currentScalarsName();
        auto component = colorMapping->currentScalars()->dataComponent();

        for (auto data : m_activeInputData)
        {
            if (!m_context.renderView().contains(data, i))
                continue;

            // don't plot the same data multiple times
            QPair<DataObject *, QString> currentPlotCombination = { data, scalars };
            if (createdPlots.contains(currentPlotCombination))
                continue;

            // TODO this should be propagated by the color mapping
            // this here is a dangerous assumption
            IndexType location = data->dataTypeName() == "polygonal mesh"
                ? IndexType::cells
                : IndexType::points;

            auto profile = std::make_unique<ImageProfileData>(
                data->name() + " plot",
                *data,
                scalars,
                location,
                component);

            if (!profile->isValid())
                continue;

            createdPlots << currentPlotCombination;

            m_previewProfiles.push_back(std::move(profile));
        }
    }

    if (m_previewProfiles.empty())
    {
        abortProfilePlot();
        return;
    }


    bool creatingNewRenderer = m_previewRenderer == nullptr;

    if (creatingNewRenderer) // if starting a new plot: create the line widget
    {
        assert(!m_lineWidget);
        m_lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
        auto repr = vtkSmartPointer<vtkLineRepresentation>::New();
        repr->SetLineColor(1, 0, 0);

        m_lineWidget->SetRepresentation(repr);
        m_lineWidget->SetInteractor(m_context.interactor());
        m_lineWidget->SetCurrentRenderer(m_context.renderer(0u));   // put the widget in the first renderer, for now
        m_lineWidget->On();

        double bounds[6];
        m_context.dataBounds(bounds, 0);
        bounds[4] = bounds[5] += g_lineZOffset;

        repr->PlaceWidget(bounds);

        m_context.render();
    }

    // applies the transformation defined by the line to the plots
    lineMoved();

    QList<DataObject *> profiles;
    for (auto && profile : m_previewProfiles)
        profiles << profile.get();

    if (creatingNewRenderer)
    {
        m_previewRenderer = DataMapping::instance().openInRenderView(profiles);
    }
    else
    {
        QList<DataObject *> incompatible;
        m_previewRenderer->showDataObjects(profiles, incompatible);
        assert(incompatible.isEmpty());
    }

    if (!m_previewRenderer) // in case the user closed the view again
    {
        abortProfilePlot();
        return;
    }

    if (creatingNewRenderer)
    {
        m_previewRendererConnections <<
            connect(m_previewRenderer, &AbstractDataView::closed, this, &RenderViewStrategy2D::abortProfilePlot);


        for (auto it = m_observerTags.begin(); it != m_observerTags.end(); ++it)
        {
            it.key()->RemoveObserver(it.value());
        }
        m_observerTags.clear();

        auto addLineObservation = [this] (vtkObject * subject)
        {
            auto tag = subject->AddObserver(vtkCommand::ModifiedEvent, this, &RenderViewStrategy2D::lineMoved);
            m_observerTags.insert(subject, tag);
        };

        addLineObservation(m_lineWidget->GetLineRepresentation()->GetLineHandleRepresentation());
        addLineObservation(m_lineWidget->GetLineRepresentation()->GetPoint1Representation());
        addLineObservation(m_lineWidget->GetLineRepresentation()->GetPoint2Representation());
    }

    m_profilePlotAcceptAction->setVisible(true);
    m_profilePlotAbortAction->setVisible(true);
}

void RenderViewStrategy2D::acceptProfilePlot()
{
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    DataSetHandler::instance().takeData(std::move(m_previewProfiles));
    m_previewProfiles.clear();
    for (auto & c : m_previewRendererConnections)
        disconnect(c);
    m_previewRendererConnections.clear();

    m_previewRenderer = nullptr;
    m_activeInputData.clear();

    m_lineWidget = nullptr;
    m_context.render();

    m_profilePlotAction->setEnabled(true);
}

void RenderViewStrategy2D::abortProfilePlot()
{
    for (auto & c : m_previewRendererConnections)
        disconnect(c);
    m_previewRendererConnections.clear();

    auto oldPreviewRenderer = m_previewRenderer;
    m_previewRenderer = nullptr;
    m_activeInputData.clear();

    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    m_lineWidget = nullptr;
    m_context.render();

    // this becomes nullptr in case the user closes the viewer while we were starting a plot
    if (oldPreviewRenderer)
        oldPreviewRenderer->close();

    m_previewProfiles.clear();

    m_profilePlotAction->setEnabled(true);
}

QString RenderViewStrategy2D::defaultInteractorStyle() const
{
    return "InteractorStyleImage";
}

void RenderViewStrategy2D::clearProfilePlots()
{
    QList<DataObject *> toDelete;
    for (auto & plot : m_previewProfiles)
    {
        toDelete << plot.get();
    }
    m_previewRenderer->prepareDeleteData(toDelete);

    m_previewProfiles.clear();
    m_activeInputData.clear();
}

void RenderViewStrategy2D::lineMoved()
{
    assert(m_previewProfiles.size() > 0 && m_lineWidget);

    double point1_[3], point2_[3];
    m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1_);
    m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2_);

    for (auto && profile : m_previewProfiles)
    {
        static_cast<ImageProfileData *>(profile.get())->setPoints(
        { point1_[0], point1_[1] },
        { point2_[0], point2_[1] });
    }
}

void RenderViewStrategy2D::updateAutomaticPlots()
{
    // don't check here, if the user of this class explicitly set the inputs
    if (!m_inputData.isEmpty())
        return;

    // refresh the automatically fetched plots
    if (m_previewRenderer)
    {
        clearProfilePlots();
        startProfilePlot();
    }

    if (m_activeInputData.isEmpty())
    {
        abortProfilePlot();
    }
}
