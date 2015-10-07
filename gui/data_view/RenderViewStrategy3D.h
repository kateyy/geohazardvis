#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategy3D : public RenderViewStrategy
{
public:
    RenderViewStrategy3D(RendererImplementationBase3D & context);

    QString name() const override;

    bool contains3dData() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

protected:
    QString defaultInteractorStyle() const override;

private:
    void updateImageWidgets();

private:
    static const bool s_isRegistered;
};
