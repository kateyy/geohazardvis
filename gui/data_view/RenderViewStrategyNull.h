#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategyNull : public RenderViewStrategy
{
public:
    RenderViewStrategyNull(RendererImplementationBase3D & context);

    QString name() const override;
    bool contains3dData() const override;
    void resetCamera(vtkCamera & camera) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;
};
