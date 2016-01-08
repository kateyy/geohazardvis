#include "RendererImplementationResidual.h"

#include <algorithm>
#include <cassert>

#include <vtkRenderer.h>

#include <core/color_mapping/ColorMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy2D.h>


RendererImplementationResidual::RendererImplementationResidual(AbstractRenderView & renderView)
    : RendererImplementationBase3D(renderView)
    , m_isInitialized(false)
{
}

RendererImplementationResidual::~RendererImplementationResidual() = default;

QString RendererImplementationResidual::name() const
{
    return "Residual Verification View";
}

void RendererImplementationResidual::activate(t_QVTKWidget & qvtkWidget)
{
    RendererImplementationBase3D::activate(qvtkWidget);

    if (m_isInitialized)
        return;

    m_strategy = std::make_unique<RenderViewStrategy2D>(*this);
    m_strategy->activate();

    setSupportedInteractionStrategies({ m_strategy->name() }, m_strategy->name());

    auto numberOfSubViews = m_renderView.numberOfSubViews();

    for (unsigned i = 0; i < numberOfSubViews; ++i)
    {
        auto ren = renderer(i);
        ren->SetViewport(  // left to right placement
            double(i) / double(numberOfSubViews), 0,
            double(i + 1) / double(numberOfSubViews), 1);
    }

    m_isInitialized = true;
}

RenderViewStrategy2D & RendererImplementationResidual::strategy2D()
{
    assert(m_strategy);

    return *m_strategy;
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

unsigned int RendererImplementationResidual::subViewIndexAtPos(const QPoint pixelCoordinate) const
{
    float uncheckedIndex = std::trunc(float(pixelCoordinate.x() * m_renderView.numberOfSubViews()) / float(m_renderView.width()));

    return static_cast<unsigned int>(
        std::max(0.0f, std::min(uncheckedIndex, float(m_renderView.numberOfSubViews() - 1u))));
}

RenderViewStrategy * RendererImplementationResidual::strategyIfEnabled() const
{
    return m_strategy.get();
}
