#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategy3D : public RenderViewStrategy
{
public:
    RenderViewStrategy3D(RenderView & renderView);

    QString name() const override;

    bool contains3dData() const override;

    void resetCamera(vtkCamera & camera) override;

    QStringList checkCompatibleObjects(QList<DataObject *> & dataObjects) const override;
    bool canApplyTo(const QList<RenderedData *> & renderedData) override;

private:
    static const bool s_isRegistered;
};
