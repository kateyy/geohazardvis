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

#include <core/vtkhelper.h>
#include <core/vtkcamerahelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/data_objects/ImageProfilePlot.h>
#include <core/DataSetHandler.h>
#include <gui/data_view/RenderView.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>

const bool RenderViewStrategyImage2D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategyImage2D>();


RenderViewStrategyImage2D::RenderViewStrategyImage2D(RenderView & renderView)
    : RenderViewStrategy(renderView)
    , m_isInitialized(false)
    , m_toolbar(nullptr)
{
}

RenderViewStrategyImage2D::~RenderViewStrategyImage2D()
{
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

    QWidget * content = m_context.findChild<QWidget *>("content", Qt::FindDirectChildrenOnly);
    content->layout()->addWidget(m_toolbar);

    m_context.axesActor()->GetLabelTextProperty(0)->SetOrientation(0);
    m_context.axesActor()->GetLabelTextProperty(1)->SetOrientation(90);
}

void RenderViewStrategyImage2D::deactivate()
{
    QWidget * content = m_context.findChild<QWidget *>("content", Qt::FindDirectChildrenOnly);

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

QStringList RenderViewStrategyImage2D::checkCompatibleObjects(QList<DataObject *> & dataObjects) const
{
    QList<DataObject *> compatible;
    QStringList incompatible;

    for (DataObject * dataObject : dataObjects)
    {
        if (dynamic_cast<ImageDataObject *>(dataObject))
            compatible << dataObject;
        else
            incompatible << dataObject->name();
    }

    dataObjects = compatible;

    return incompatible;
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
    m_lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
    VTK_CREATE(vtkLineRepresentation, repr);
    repr->SetLineColor(1, 0, 0);
    m_lineWidget->SetRepresentation(repr);
    m_lineWidget->SetInteractor(m_context.renderWindow()->GetInteractor());
    m_lineWidget->On();

    double bounds[6];

    m_context.getDataBounds(bounds);

    bounds[4] += 0.0001;
    bounds[5] += 0.0001;

    repr->PlaceWidget(bounds);

    m_context.render();

    m_profilePlotAction->setEnabled(false);
    m_profilePlotAcceptAction->setVisible(true);
    m_profilePlotAbortAction->setVisible(true);
}

void RenderViewStrategyImage2D::acceptProfilePlot()
{
    ImageDataObject * image = nullptr;
    for (DataObject * dataObject : m_context.dataObjects())
    {
        image = dynamic_cast<ImageDataObject *>(dataObject);
        if (image)
            break;
    }
    assert(image);

    double point1[3], point2[3];
    m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1);
    m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2);
    point1[2] = point2[2] = 0;

    QString name = image->name() + " plot";
    ImageProfileData * profile = new ImageProfileData(name, image, point1, point2);

    DataSetHandler::instance().addData({ profile });

    m_lineWidget = nullptr;
    m_context.render();

    m_profilePlotAction->setEnabled(true);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);
}

void RenderViewStrategyImage2D::abortProfilePlot()
{
    m_lineWidget = nullptr;
    m_context.render();

    m_profilePlotAction->setEnabled(true);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);
}
