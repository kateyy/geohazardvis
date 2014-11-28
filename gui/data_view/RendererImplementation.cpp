#include "RendererImplementation.h"


RendererImplementation::RendererImplementation(RenderView & renderView, QObject * parent)
    : QObject(parent)
    , m_renderView(renderView)
{
}

RendererImplementation::~RendererImplementation()
{
}

RenderView & RendererImplementation::renderView() const
{
    return m_renderView;
}

void RendererImplementation::activate(QVTKWidget * /*qvtkWidget*/)
{
}

void RendererImplementation::deactivate(QVTKWidget * /*qvtkWidget*/)
{
}

const QList<RendererImplementation::ImplementationConstructor> & RendererImplementation::constructors()
{
    return s_constructors();
}

QList<RendererImplementation::ImplementationConstructor> & RendererImplementation::s_constructors()
{
    static QList<ImplementationConstructor> list;

    return list;
}
