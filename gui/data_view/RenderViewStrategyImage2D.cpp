#include "RenderViewStrategyImage2D.h"

#include <vtkCamera.h>

#include <core/vtkcamerahelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <gui/data_view/RenderView.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>

const bool RenderViewStrategyImage2D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategyImage2D>();


RenderViewStrategyImage2D::RenderViewStrategyImage2D(RenderView & renderView)
    : RenderViewStrategy(renderView)
{
}

QString RenderViewStrategyImage2D::name() const
{
    return "2D image";
}

void RenderViewStrategyImage2D::activate()
{
    m_context.interactorStyleSwitch()->setCurrentStyle("InteractorStyleImage");
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
