#pragma once

#include <array>

#include <gui/data_view/RendererImplementationBase3D.h>


class RendererImplementationResidual : public RendererImplementationBase3D
{
public:
    RendererImplementationResidual(AbstractRenderView & renderView);
    ~RendererImplementationResidual() override;

    QString name() const override;

protected:
    ColorMapping * colorMappingForSubView(unsigned int subViewIndex) override;

private:
    std::array<std::unique_ptr<ColorMapping>, 3> m_colorMappings;
};
