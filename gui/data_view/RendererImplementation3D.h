#pragma once

#include <memory>

#include <gui/data_view/RendererImplementationBase3D.h>


class RenderViewStrategySwitch;


class RendererImplementation3D : public RendererImplementationBase3D
{
    Q_OBJECT

public:
    RendererImplementation3D(AbstractRenderView & renderView);
    ~RendererImplementation3D() override;

    QString name() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget * qvtkWidget) override;

signals:
    void resetStrategy(const QList<DataObject *> & dataObjects);

protected:
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onDataVisibilityChanged(AbstractVisualizedData * content, unsigned int subViewIndex) override;

    ColorMapping * colorMappingForSubView(unsigned int subViewIndex) override;

private:
    std::unique_ptr<RenderViewStrategySwitch> m_strategySwitch;
    std::unique_ptr<ColorMapping> m_colorMapping;

    static bool s_isRegistered;
};