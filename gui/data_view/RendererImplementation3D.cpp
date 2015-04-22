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
    , m_strategySwitch(nullptr)
{
}

RendererImplementation3D::~RendererImplementation3D()
{
    delete m_strategySwitch;
}

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
        m_strategySwitch = new RenderViewStrategySwitch(*this);

    connect(&m_renderView, &AbstractRenderView::visualizationsChanged,
        this, &RendererImplementation3D::updateColorMapping);
}

void RendererImplementation3D::deactivate(QVTKWidget * qvtkWidget)
{
    disconnect(&m_renderView, &AbstractRenderView::visualizationsChanged,
        this, &RendererImplementation3D::updateColorMapping);

    RendererImplementationBase3D::deactivate(qvtkWidget);
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

void RendererImplementation3D::updateColorMapping()
{
    colorMapping()->setVisualizedData(m_renderView.visualizations());
    if (interactorStyle())
        interactorStyle()->setRenderedData(renderedData());
}
