#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategy3D : public RenderViewStrategy
{
public:
    explicit RenderViewStrategy3D(RendererImplementationBase3D & context);
    ~RenderViewStrategy3D() override;

    QString name() const override;

    bool contains3dData() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

protected:
    QString defaultInteractorStyle() const override;

private:
    void updateImageWidgets();

private:
    static const bool s_isRegistered;

private:
    Q_DISABLE_COPY(RenderViewStrategy3D)
};
