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
        auto vectors = creator.second(renderedData);

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

bool GlyphMappingRegistry::registerImplementation(const QString & name, const MappingCreator & creator)
{
    auto it = m_mappingCreators.find(name);
    assert(it == m_mappingCreators.end());
    if (it != m_mappingCreators.end())
    {
        return false;
    }

    m_mappingCreators.emplace(name, creator);
    return true;
}
