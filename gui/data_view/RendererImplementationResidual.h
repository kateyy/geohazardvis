#pragma once

#include <gui/data_view/RendererImplementationBase3D.h>


class RenderViewStrategy2D;


class GUI_API RendererImplementationResidual : public RendererImplementationBase3D
{
public:
    explicit RendererImplementationResidual(AbstractRenderView & renderView);
    ~RendererImplementationResidual() override;

    QString name() const override;

    void activate(t_QVTKWidget & qvtkWidget) override;

    RenderViewStrategy2D & strategy2D();

protected:
    unsigned int subViewIndexAtPos(const QPoint pixelCoordinate) const override;

    RenderViewStrategy * strategyIfEnabled() const override;

private:
    bool m_isInitialized;

    std::unique_ptr<RenderViewStrategy2D> m_strategy;
};
