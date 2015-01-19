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

QMap<QString, GlyphMappingData *> GlyphMappingRegistry::createMappingsValidFor(RenderedData * renderedData)
{
    QMap<QString, GlyphMappingData *> validVectors;

    for (auto creator : m_mappingCreators)
    {
        QList<GlyphMappingData *> vectors = creator(renderedData);

        for (GlyphMappingData * v : vectors)
        {
            if (v->isValid())
                validVectors.insert(v->name(), v);
            else
                delete v;
        }
    }

    return validVectors;
}

const QMap<QString, GlyphMappingRegistry::MappingCreator> & GlyphMappingRegistry::mappingCreators()
{
    return m_mappingCreators;
}

bool GlyphMappingRegistry::registerImplementation(QString name, const MappingCreator & constructor)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
        return false;

    m_mappingCreators.insert(name, constructor);
    return true;
}
