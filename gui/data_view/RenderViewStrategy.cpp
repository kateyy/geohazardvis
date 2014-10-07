#include "RenderViewStrategy.h"


RenderViewStrategy::RenderViewStrategy(RenderView & renderView)
    : m_context(renderView)
{
}

RenderViewStrategy::~RenderViewStrategy() = default;

const QList<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::constructors()
{
    return s_constructors();
}

QList<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::s_constructors()
{
    static QList<StategyConstructor> list;

    return list;
}