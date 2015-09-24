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

void RenderViewStrategy3D::activate()
{
    m_context.interactorStyleSwitch()->setCurrentStyle("InteractorStyle3D");
}

bool RenderViewStrategy3D::contains3dData() const
{
    return true;
}

void RenderViewStrategy3D::resetCamera()
{
    m_context.interactorStyleSwitch()->resetCamera();
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
        if (!rendered->dataObject().is3D())
            return false;

    return true;
}

void RenderViewStrategy3D::updateImageWidgets()
{
    for (RenderedData * r : m_context.renderedData())
    {
        if (RenderedVectorGrid3D * grid = dynamic_cast<RenderedVectorGrid3D *>(r))
            grid->setRenderWindowInteractor(m_context.interactor());
    }
}
