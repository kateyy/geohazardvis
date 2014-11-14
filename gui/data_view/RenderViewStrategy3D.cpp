#include "RenderViewStrategy3D.h"

#include <vtkCamera.h>

#include <core/vtkcamerahelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <gui/data_view/RendererImplementation3D.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>


const bool RenderViewStrategy3D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategy3D>();


RenderViewStrategy3D::RenderViewStrategy3D(RendererImplementation3D & context)
    : RenderViewStrategy(context)
{
}

QString RenderViewStrategy3D::name() const
{
    return "3D terrain";
}

void RenderViewStrategy3D::activate()
{
    m_context.interactorStyleSwitch()->setCurrentStyle("InteractorStyle3D");
}

bool RenderViewStrategy3D::contains3dData() const
{
    return true;
}

void RenderViewStrategy3D::resetCamera(vtkCamera & camera)
{
    camera.SetViewUp(0, 0, 1);
    TerrainCamera::setAzimuth(camera, 0);
    TerrainCamera::setVerticalElevation(camera, 45);
}

QList<DataObject *> RenderViewStrategy3D::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const
{
    QList<DataObject *> compatible;

    for (DataObject * dataObject : dataObjects)
    {
        if (dataObject->is3D())
            compatible << dataObject;
        else
            incompatibleObjects << dataObject;
    }

    return compatible;
}

bool RenderViewStrategy3D::canApplyTo(const QList<RenderedData *> & renderedData)
{
    for (RenderedData * rendered : renderedData)
        if (!rendered->dataObject()->is3D())
            return false;

    return true;
}
