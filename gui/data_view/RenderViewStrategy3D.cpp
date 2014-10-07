#include "RenderViewStrategy3D.h"

#include <vtkCamera.h>

#include <core/vtkcamerahelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <gui/data_view/RenderView.h>
#include <gui/rendering_interaction/PickingInteractorStyleSwitch.h>


const bool RenderViewStrategy3D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategy3D>();


RenderViewStrategy3D::RenderViewStrategy3D(RenderView & renderView)
    : RenderViewStrategy(renderView)
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

QStringList RenderViewStrategy3D::checkCompatibleObjects(QList<DataObject *> & dataObjects) const
{
    QList<DataObject *> compatible;
    QStringList incompatible;

    for (DataObject * dataObject : dataObjects)
    {
        if (dataObject->is3D())
            compatible << dataObject;
        else
            incompatible << dataObject->name();
    }

    dataObjects = compatible;

    return incompatible;
}

bool RenderViewStrategy3D::canApplyTo(const QList<RenderedData *> & renderedData)
{
    for (RenderedData * rendered : renderedData)
        if (!rendered->dataObject()->is3D())
            return false;

    return true;
}
