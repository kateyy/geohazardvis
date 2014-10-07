#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategyNull : public RenderViewStrategy
{
public:
    RenderViewStrategyNull(RenderView & renderView);

    QString name() const override;
    bool contains3dData() const override;
    void resetCamera(vtkCamera & camera) override;
    QStringList checkCompatibleObjects(QList<DataObject *> & dataObjects) const override;
    bool canApplyTo(const QList<RenderedData *> & renderedData) override;
};
