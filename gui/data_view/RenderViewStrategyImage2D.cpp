#include "RenderViewStrategyImage2D.h"

#include <cassert>

#include <QAction>
#include <QIcon>
#include <QLayout>
#include <QToolBar>

#include <vtkCamera.h>
#include <vtkRenderWindow.h>

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
        if (!dataObject->is3D())
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
        if (rendered->dataObject()->is3D())
            return false;

    return true;
}

void RenderViewStrategyImage2D::startProfilePlot()
{
    m_lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
    VTK_CREATE(vtkLineRepresentation, repr);
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
    ImageDataObject * imageData = dynamic_cast<ImageDataObject *>(m_context.dataObjects().first());
    if (!imageData)
        return;

    double point1[3], point2[3];
    m_lineWidget->GetLineRepresentation()->GetPoint1WorldPosition(point1);
    m_lineWidget->GetLineRepresentation()->GetPoint2WorldPosition(point2);
    ImageProfileData * profile = new ImageProfileData(imageData, point1, point2);

    DataSetHandler::instance().addData({ profile });

    m_lineWidget = nullptr;

    m_profilePlotAction->setEnabled(true);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);
}

void RenderViewStrategyImage2D::abortProfilePlot()
{
    m_lineWidget = nullptr;

    m_profilePlotAction->setEnabled(true);
    m_profilePlotAcceptAction->setVisible(false);
    m_profilePlotAbortAction->setVisible(false);
}
