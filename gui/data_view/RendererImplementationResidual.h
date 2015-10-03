#pragma once

#include <array>

#include <gui/data_view/RendererImplementationBase3D.h>


class GUI_API RendererImplementationResidual : public RendererImplementationBase3D
{
public:
    RendererImplementationResidual(AbstractRenderView & renderView);
    ~RendererImplementationResidual() override;

    QString name() const override;

    void activate(QVTKWidget & qvtkWidget) override;

protected:
    ColorMapping * colorMappingForSubView(unsigned int subViewIndex) override;

    unsigned int subViewIndexAtPos(const QPoint pixelCoordinate) const override;

private:
    bool m_isInitialized;

    std::array<std::unique_ptr<ColorMapping>, 3> m_colorMappings;
};
