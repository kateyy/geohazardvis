#include "RendererImplementationNull.h"

#include <core/types.h>


RendererImplementationNull::RendererImplementationNull(RenderView & renderView, QObject * parent)
    : RendererImplementation(renderView, parent)
{
}

ContentType RendererImplementationNull::contentType() const
{
    return ContentType::invalid;
}
