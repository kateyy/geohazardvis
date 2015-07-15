#include "RendererImplementation3D.h"

#include <cassert>

#include <core/color_mapping/ColorMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>
#include <gui/data_view/RenderViewStrategySwitch.h>
#include <gui/rendering_interaction/IPickingInteractorStyle.h>


bool RendererImplementation3D::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementation3D>();


RendererImplementation3D::RendererImplementation3D(AbstractRenderView & renderView, QObject * parent)
    : RendererImplementationBase3D(renderView, parent)
{
}

RendererImplementation3D::~RendererImplementation3D() = default;

QList<DataObject *> RendererImplementation3D::filterCompatibleObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects)
{
    if (!m_strategy)
        emit resetStrategy(dataObjects);

    return strategy().filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementation3D::activate(QVTKWidget * qvtkWidget)
{
    RendererImplementationBase3D::activate(qvtkWidget);

    if (!m_strategySwitch)
        m_strategySwitch = std::make_unique<RenderViewStrategySwitch>(*this);
}

void RendererImplementation3D::onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    RendererImplementationBase3D::onRemoveContent(content, subViewIndex);

    // reset strategy if we are empty
    if (!viewportSetup(subViewIndex).dataBounds.IsValid())
        setStrategy(nullptr);
}

void RendererImplementation3D::onDataVisibilityChanged(AbstractVisualizedData * /*content*/, unsigned int subViewIndex)
{
    // reset strategy if we are empty
    if (!viewportSetup(subViewIndex).dataBounds.IsValid())
        setStrategy(nullptr);
}

void RendererImplementation3D::onRenderViewVisualizationChanged()
{
    RendererImplementationBase3D::onRenderViewVisualizationChanged();

    if (interactorStyle())
        interactorStyle()->setRenderedData(renderedData());
}
