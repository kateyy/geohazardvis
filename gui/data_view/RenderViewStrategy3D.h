#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategy3D : public RenderViewStrategy
{
public:
    RenderViewStrategy3D(RendererImplementationBase3D & context);

    QString name() const override;

    void activate() override;

    bool contains3dData() const override;

    void resetCamera(vtkCamera & camera) override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

private:
    void updateImageWidgets();

private:
    static const bool s_isRegistered;
};
