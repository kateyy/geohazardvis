#include "RenderViewStrategyNull.h"


RenderViewStrategyNull::RenderViewStrategyNull(RendererImplementationBase3D & context)
    : RenderViewStrategy(context)
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

void RenderViewStrategyNull::resetCamera(vtkCamera &)
{
}

QList<DataObject *> RenderViewStrategyNull::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const
{
    incompatibleObjects = dataObjects;

    return{};
}
