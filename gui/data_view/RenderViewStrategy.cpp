#include "RenderViewStrategy.h"


RenderViewStrategy::RenderViewStrategy(RendererImplementationBase3D & context, QObject * parent)
    : QObject(parent)
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

const QList<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::constructors()
{
    return s_constructors();
}

QList<RenderViewStrategy::StategyConstructor> & RenderViewStrategy::s_constructors()
{
    static QList<StategyConstructor> list;

    return list;
}