#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategyImage2D : public RenderViewStrategy
{
public:
    RenderViewStrategyImage2D(RenderView & renderView);

    QString name() const override;

    void activate() override;

    bool contains3dData() const override;

    void resetCamera(vtkCamera & camera) override;

    QStringList checkCompatibleObjects(QList<DataObject *> & dataObjects) const override;
    bool canApplyTo(const QList<RenderedData *> & renderedData) override;

private:
    static const bool s_isRegistered;
};
