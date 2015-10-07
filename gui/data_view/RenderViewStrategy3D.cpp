#include "RenderViewStrategy3D.h"

#include <vtkCamera.h>

#include <core/utility/vtkcamerahelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/rendering_interaction/CameraInteractorStyleSwitch.h>


const bool RenderViewStrategy3D::s_isRegistered = RenderViewStrategy::registerStrategy<RenderViewStrategy3D>();


RenderViewStrategy3D::RenderViewStrategy3D(RendererImplementationBase3D & context)
    : RenderViewStrategy(context)
{
    connect(&context.renderView(), &AbstractRenderView::visualizationsChanged, this, &RenderViewStrategy3D::updateImageWidgets);
}

QString RenderViewStrategy3D::name() const
{
    return "3D terrain";
}

QString RenderViewStrategy3D::defaultInteractorStyle() const
{
    return "InteractorStyleTerrain";
}

bool RenderViewStrategy3D::contains3dData() const
{
    return true;
}

void RenderViewStrategy3D::resetCamera(vtkCamera & camera)
{
    m_context.interactorStyleSwitch()->resetCamera(camera);
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

void RenderViewStrategy3D::updateImageWidgets()
{
    for (RenderedData * r : m_context.renderedData())
    {
        if (RenderedVectorGrid3D * grid = dynamic_cast<RenderedVectorGrid3D *>(r))
            grid->setRenderWindowInteractor(m_context.interactor());
    }
}
