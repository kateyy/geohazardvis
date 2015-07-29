#include "RendererImplementationResidual.h"

#include <core/color_mapping/ColorMapping.h>


RendererImplementationResidual::RendererImplementationResidual(AbstractRenderView & renderView)
    : RendererImplementationBase3D(renderView)
{
}

RendererImplementationResidual::~RendererImplementationResidual() = default;

QString RendererImplementationResidual::name() const
{
    return "Residual Verification View";
}

ColorMapping * RendererImplementationResidual::colorMappingForSubView(unsigned int subViewIndex)
{
    auto && colorMapping = m_colorMappings[subViewIndex];
    if (!colorMapping)
    {
        colorMapping = std::make_unique<ColorMapping>();
    }

    return colorMapping.get();
}
