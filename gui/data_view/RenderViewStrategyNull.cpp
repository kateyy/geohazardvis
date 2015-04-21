#include "RenderViewStrategyNull.h"


RenderViewStrategyNull::RenderViewStrategyNull(RendererImplementationBase3D & context, QObject * parent)
    : RenderViewStrategy(context, parent)
{
}

QString RenderViewStrategyNull::name() const
{
    return "";
}

bool RenderViewStrategyNull::contains3dData() const
{
    return true;
}

void RenderViewStrategyNull::resetCamera(vtkCamera & /*camera*/)
{
}

QList<DataObject *> RenderViewStrategyNull::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const
{
    incompatibleObjects = dataObjects;

    return{};
}

bool RenderViewStrategyNull::canApplyTo(const QList<RenderedData *> & /*renderedData*/)
{
    return false;
}
