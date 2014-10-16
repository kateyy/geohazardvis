#include "RenderViewStrategyImage2DPlot.h"

#include <cassert>

#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkCubeAxesActor.h>
#include <vtkTextProperty.h>

#include <core/data_objects/ImageProfileData.h>
#include <core/data_objects/ImageProfilePlot.h>
#include <gui/data_view/RenderView.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>


const bool RenderViewStrategyImage2DPlot::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategyImage2DPlot>();


RenderViewStrategyImage2DPlot::RenderViewStrategyImage2DPlot(RenderView & renderView)
    : RenderViewStrategy(renderView)
    , m_isInitialized(false)
{
}

RenderViewStrategyImage2DPlot::~RenderViewStrategyImage2DPlot()
{
}

void RenderViewStrategyImage2DPlot::initialize()
{
    if (m_isInitialized)
        return;

    m_isInitialized = true;
}

QString RenderViewStrategyImage2DPlot::name() const
{
    return "2D image plot";
}

void RenderViewStrategyImage2DPlot::activate()
{
    initialize();

    m_context.interactorStyleSwitch()->setCurrentStyle("InteractorStyleImage");


    vtkCubeAxesActor & axes = *m_context.axesActor();
    axes.GetLabelTextProperty(0)->SetOrientation(0);
    axes.GetLabelTextProperty(1)->SetOrientation(90);

}

void RenderViewStrategyImage2DPlot::deactivate()
{
}

bool RenderViewStrategyImage2DPlot::contains3dData() const
{
    return false;
}

void RenderViewStrategyImage2DPlot::resetCamera(vtkCamera & camera)
{
    camera.SetViewUp(0, 1, 0);
    camera.SetFocalPoint(0, 0, 0);
    camera.SetPosition(0, 0, 1);

    ImageProfileData * profile = dynamic_cast<ImageProfileData *>(m_context.dataObjects().first());
    QString scalarsName;
    QString abscissa;
    if (profile)
    {
        abscissa = profile->abscissa();
        scalarsName = profile->scalarsName();
    }

    vtkCubeAxesActor & axes = *m_context.axesActor();
    axes.SetXTitle(abscissa.toLatin1().data());
    axes.SetYTitle(scalarsName.toLatin1().data());
}

QList<DataObject *> RenderViewStrategyImage2DPlot::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const
{
    QList<DataObject *> compatible;

    for (DataObject * dataObject : dataObjects)
    {
        if (dynamic_cast<ImageProfileData *>(dataObject))
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;
    }

    return compatible;
}

bool RenderViewStrategyImage2DPlot::canApplyTo(const QList<RenderedData *> & renderedData)
{
    for (RenderedData * rendered : renderedData)
        if (!dynamic_cast<ImageProfilePlot *>(rendered))
            return false;

    return true;
}
