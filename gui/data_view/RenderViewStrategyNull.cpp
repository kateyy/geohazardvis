#include "RenderViewStrategyNull.h"


RenderViewStrategyNull::RenderViewStrategyNull(RenderView & renderView)
    : RenderViewStrategy(renderView)
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

QStringList RenderViewStrategyNull::checkCompatibleObjects(QList<DataObject *> & /*dataObjects*/) const
{
    return{};
}

bool RenderViewStrategyNull::canApplyTo(const QList<RenderedData *> & /*renderedData*/)
{
    return false;
}
