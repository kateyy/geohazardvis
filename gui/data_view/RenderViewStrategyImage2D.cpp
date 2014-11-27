#include "RenderViewStrategyImage2D.h"

#include <cassert>

#include <QAction>
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
#include <core/vtkhelper.h>
#include <core/vtkcamerahelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/rendered_data/ImageProfilePlot.h>
#include <gui/DataMapping.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementation3D.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>

const bool RenderViewStrategyImage2D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategyImage2D>();


RenderViewStrategyImage2D::RenderViewStrategyImage2D(RendererImplementation3D & context)
    : RenderViewStrategy(context)
    , m_isInitialized(false)
    , m_toolbar(nullptr)
    , m_previewProfile(nullptr)
    , m_previewRenderer(nullptr)
    , m_currentPlottingImage(nullptr)
{
    connect(&context.renderView(), &RenderView::contentChanged, this, &RenderViewStrategyImage2D::checkSourceExists);
}

RenderViewStrategyImage2D::~RenderViewStrategyImage2D()
{
    if (m_previewRenderer && m_previewProfile)
    {
        m_previewRenderer->removeDataObjects({ m_previewProfile });
        if (m_previewRenderer->contents().isEmpty())
            m_previewRenderer->close();
    }

    delete m_previewProfile;

    delete m_toolbar;
}

void RenderViewStrategyImage2D::initialize()
{
    if (m_isInitialized)
        return;

    m_toolbar = new QToolBar();

    m_toolbar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
    m_profilePlotAction = m_toolbar->addAction(QIcon(":/icons/graph_line"), "create &profile plot", this, SLOT(startProfilePlot()));
    m_profilePlotAcceptAction = m_toolbar->addAction(QIcon(":/icons/yes"), "", this, SLOT(acceptProfilePlot()));
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction = m_toolbar->addAction(QIcon(":/icons/no"), "", this, SLOT(abortProfilePlot()));
    m_profilePlotAbortAction->setVisible(false);

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

    QWidget * content = m_context.renderView().findChild<QWidget *>("content", Qt::FindDirectChildrenOnly);
    content->layout()->addWidget(m_toolbar);

    m_context.axesActor()->GetLabelTextProperty(0)->SetOrientation(0);
    m_context.axesActor()->GetLabelTextProperty(1)->SetOrientation(90);
}

void RenderViewStrategyImage2D::deactivate()
{
    QWidget * content = m_context.renderView().findChild<QWidget *>("content", Qt::FindDirectChildrenOnly);

    content->layout()->removeWidget(m_toolbar);
    m_toolbar->setParent(nullptr);
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
    assert(!m_previewProfile && !m_previewRenderer);

    m_profilePlotAction->setEnabled(false);
    m_profilePlotAcceptAction->setVisible(true);
    m_profilePlotAbortAction->setVisible(true);

    // place the line widget
    
    m_lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
    VTK_CREATE(vtkLineRepresentation, repr);
    repr->SetLineColor(1, 0, 0);
    m_lineWidget->SetRepresentation(repr);
    m_lineWidget->SetInteractor(m_context.interactor());
    m_lineWidget->On();

    double bounds[6];

    m_context.dataBounds(bounds);

    bounds[4] += 0.0001;
    bounds[5] += 0.0001;

    repr->PlaceWidget(bounds);

    m_context.render();



    // create profile preview

    assert(!m_currentPlottingImage);
    ImageDataObject * image = nullptr;
    for (DataObject * dataObject : m_context.renderView().dataObjects())
    {
        image = dynamic_cast<ImageDataObject *>(dataObject);
        if (image)
            break;
    }
    assert(image);
    m_currentPlottingImage = image;

    QString name = m_currentPlottingImage->name() + " plot";
    m_previewProfile = new ImageProfileData(name, m_currentPlottingImage);
    lineMoved();

    m_previewRenderer = DataMapping::instance().openInRenderView({ m_previewProfile });
    connect(m_previewRenderer, &RenderView::closed, this, &RenderViewStrategyImage2D::abortProfilePlot);

    m_vtkConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    m_vtkConnect->Connect(m_lineWidget->GetLineRepresentation()->GetLineHandleRepresentation(), vtkCommand::ModifiedEvent, this, SLOT(lineMoved()));
    m_vtkConnect->Connect(m_lineWidget->GetLineRepresentation()->GetPoint1Representation(), vtkCommand::ModifiedEvent, this, SLOT(lineMoved()));
    m_vtkConnect->Connect(m_lineWidget->GetLineRepresentation()->GetPoint2Representation(), vtkCommand::ModifiedEvent, this, SLOT(lineMoved()));
}

void RenderViewStrategyImage2D::acceptProfilePlot()
{
    m_profilePlotAction->setEnabled(true);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    DataSetHandler::instance().addData({ m_previewProfile });
    m_previewProfile = nullptr;
    m_previewRenderer = nullptr;
    m_currentPlottingImage = nullptr;

    m_lineWidget = nullptr;
    m_context.render();
}

void RenderViewStrategyImage2D::abortProfilePlot()
{
    if (!m_previewRenderer)
        return; // already aborting

    auto oldPreviewRenderer = m_previewRenderer;
    m_previewRenderer = nullptr;
    m_currentPlottingImage = nullptr;

    m_profilePlotAction->setEnabled(true);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);

    m_lineWidget = nullptr;
    m_context.render();

    oldPreviewRenderer->close();

    delete m_previewProfile;
    m_previewProfile = nullptr;
}

void RenderViewStrategyImage2D::lineMoved()
{
    assert(m_previewProfile && m_lineWidget);

    double point1[3], point2[3];
    m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1);
    m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2);
    point1[2] = point2[2] = 0;

    m_previewProfile->setPoints(point1, point2);
}

void RenderViewStrategyImage2D::checkSourceExists()
{
    if (!m_currentPlottingImage)
        return;

    if (!m_context.renderView().dataObjects().contains(m_currentPlottingImage))
        abortProfilePlot();
}
