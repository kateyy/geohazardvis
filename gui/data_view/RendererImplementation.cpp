#include "RendererImplementation.h"


RendererImplementation::RendererImplementation(RenderView & renderView, QObject * parent)
    : QObject(parent)
    , m_renderView(renderView)
{
}

RenderView & RendererImplementation::renderView() const
{
    return m_renderView;
}
