#include "RendererImplementation3D.h"

#include <cassert>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>


bool RendererImplementation3D::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementation3D>();


RendererImplementation3D::RendererImplementation3D(AbstractRenderView & renderView)
    : RendererImplementationBase3D(renderView)
    , m_currentStrategy(nullptr)
{
}

RendererImplementation3D::~RendererImplementation3D()
{
    if (m_currentStrategy)
    {
        m_currentStrategy->deactivate();
    }
}

QString RendererImplementation3D::name() const
{
    return "Renderer 3D";
}

QList<DataObject *> RendererImplementation3D::filterCompatibleObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects)
{
    updateStrategies(dataObjects);

    return strategy().filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementation3D::activate(QVTKWidget & qvtkWidget)
{
    RendererImplementationBase3D::activate(qvtkWidget);
}

void RendererImplementation3D::onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex)
{
    RendererImplementationBase3D::onRemoveContent(content, subViewIndex);

    updateStrategies();
}

ColorMapping * RendererImplementation3D::colorMappingForSubView(unsigned int /*subViewIndex*/)
{
    if (!m_colorMapping)
    {
        m_colorMapping = std::make_unique<ColorMapping>();
    }
    return m_colorMapping.get();
}

void RendererImplementation3D::updateForCurrentInteractionStrategy(const QString & strategyName)
{
    auto it = m_strategies.find(strategyName);
    RenderViewStrategy * newStrategy = it == m_strategies.end()
        ? nullptr
        : it->second.get();

    if (newStrategy == m_currentStrategy)
    {
        return;
    }

    if (m_currentStrategy)
    {
        m_currentStrategy->deactivate();
    }

    m_currentStrategy = newStrategy;

    if (m_currentStrategy)
    {
        m_currentStrategy->activate();
    }
}

RenderViewStrategy * RendererImplementation3D::strategyIfEnabled() const
{
    return m_currentStrategy;
}

void RendererImplementation3D::updateStrategies(const QList<DataObject *> & newDataObjects)
{
    QStringList newNames;

    if (m_strategies.empty())
    {

        for (const RenderViewStrategy::StategyConstructor & constructor : RenderViewStrategy::constructors())
        {
            auto instance = constructor(*this);

            auto && name = instance->name();
            assert(m_strategies.find(name) == m_strategies.end());
            m_strategies.emplace(name, std::move(instance));
            newNames << name;
        }
    }

    bool wasEmpty = m_renderView.visualizations().isEmpty();

    if (wasEmpty)
    {
        // reset the strategy if we were empty

        auto && current = mostSuitableStrategy(newDataObjects);

        if (newNames.isEmpty())
        {
            setInteractionStrategy(current);
        }
        else
        {
            setSupportedInteractionStrategies(newNames, current);
        }
    }
}

QString RendererImplementation3D::mostSuitableStrategy(const QList<DataObject *> & newDataObjects) const
{
    auto && dataObjects = m_renderView.dataObjects() + newDataObjects;

    // use 2D interaction by default, if there is a 2D image in our view
    // viewing 2D images in a 3D terrain view is probably not what we want in most cases

    bool contains2D = false;

    for (auto && data : dataObjects)
    {
        if (!dynamic_cast<ImageDataObject *>(data))
            continue;

        contains2D = true;
        break;
    }

    if (contains2D)
    {
        return "2D image";
    }

    return "3D terrain";
}
