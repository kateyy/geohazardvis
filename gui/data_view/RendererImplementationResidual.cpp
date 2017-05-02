#include "RendererImplementationResidual.h"

#include <algorithm>
#include <cassert>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy2D.h>


RendererImplementationResidual::RendererImplementationResidual(AbstractRenderView & renderView)
    : RendererImplementationBase3D(renderView)
    , m_isInitialized{ false }
    , m_showModel{ true }
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
    {
        return;
    }

    m_strategy = std::make_unique<RenderViewStrategy2D>(*this);
    m_strategy->activate();
    
    setSupportedInteractionStrategies({ m_strategy->name() }, m_strategy->name());

    setupRenderers();
}

void RendererImplementationResidual::setShowModel(const bool showModel)
{
    if (showModel == m_showModel)
    {
        return;
    }

    m_showModel = showModel;

    setupRenderers();
}

bool RendererImplementationResidual::showModel() const
{
    return m_showModel;
}

RenderViewStrategy2D & RendererImplementationResidual::strategy2D()
{
    assert(m_strategy);

    return *m_strategy;
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

void RendererImplementationResidual::setupRenderers()
{
    auto renWinPtr = renderWindow();
    if (!renWinPtr)
    {
        return;
    }
    auto & renWin = *renWinPtr;

    const unsigned int modelIndex = 1u;
    const auto modelRenderer = renderer(modelIndex);
    if (bool(renWin.HasRenderer(modelRenderer)) != m_showModel)
    {
        if (m_showModel)
        {
            renWin.AddRenderer(modelRenderer);
        }
        else
        {
            renWin.RemoveRenderer(modelRenderer);
        }
    }

    const unsigned int numberOfSubViews = m_renderView.numberOfSubViews();
    const unsigned int activeViews = numberOfSubViews - (m_showModel ? 0u : 1u);

    for (unsigned i = 0u; i < numberOfSubViews; ++i)
    {
        unsigned visibleIndex = i;
        if (!m_showModel)
        {
            if (i == modelIndex)
            {
                continue;
            }
            else if (i > modelIndex) // affects views right of the model view
            {
                visibleIndex -= 1u;
            }
        }

        auto ren = renderer(i);
        assert(ren);
        ren->SetViewport(  // left to right placement
            double(visibleIndex) / double(activeViews), 0,
            double(visibleIndex + 1) / double(activeViews), 1);
    }
}
