#include "GlyphMappingRegistry.h"

#include <cassert>

#include "GlyphMappingData.h"


GlyphMappingRegistry::GlyphMappingRegistry()
{
}

GlyphMappingRegistry::~GlyphMappingRegistry() = default;

GlyphMappingRegistry & GlyphMappingRegistry::instance()
{
    static GlyphMappingRegistry registry;
    return registry;
}

std::map<QString, std::unique_ptr<GlyphMappingData>> GlyphMappingRegistry::createMappingsValidFor(RenderedData & renderedData) const
{
    std::map<QString, std::unique_ptr<GlyphMappingData>> validVectors;

    for (auto && creator : m_mappingCreators)
    {
        auto vectors = creator(renderedData);

        for (auto & v : vectors)
        {
            if (v->isValid())
            {
                validVectors.emplace(v->name(), std::move(v));
            }
        }
    }

    return validVectors;
}

const QMap<QString, GlyphMappingRegistry::MappingCreator> & GlyphMappingRegistry::mappingCreators() const
{
    return m_mappingCreators;
}

bool GlyphMappingRegistry::registerImplementation(const QString & name, const MappingCreator & creator)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
    {
        return false;
    }

    m_mappingCreators.insert(name, creator);
    return true;
}
