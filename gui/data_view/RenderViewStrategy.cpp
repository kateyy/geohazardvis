#include "RenderViewStrategy.h"


RenderViewStrategy::RenderViewStrategy(RendererImplementationBase3D & context)
    : QObject()
    , m_context(context)
{
}

RenderViewStrategy::~RenderViewStrategy() = default;

void RenderViewStrategy::activate()
{
}

void RenderViewStrategy::deactivate()
{
}

const std::vector<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::constructors()
{
    return s_constructors();
}

std::vector<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::s_constructors()
{
    static std::vector<StategyConstructor> list;

    return list;
}
