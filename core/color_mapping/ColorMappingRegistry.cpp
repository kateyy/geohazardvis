#include "ColorMappingRegistry.h"

#include <cassert>

#include <core/color_mapping/ColorMappingData.h>


ColorMappingRegistry::ColorMappingRegistry()
{
}

ColorMappingRegistry::~ColorMappingRegistry() = default;

ColorMappingRegistry & ColorMappingRegistry::instance()
{
    static ColorMappingRegistry registry;
    return registry;
}

std::map<QString, std::unique_ptr<ColorMappingData>> ColorMappingRegistry::createMappingsValidFor(const QList<AbstractVisualizedData*> & visualizedData) const
{
    std::map<QString, std::unique_ptr<ColorMappingData>> validScalars;

    for (auto creator : m_mappingCreators)
    {
        auto scalars = creator(visualizedData);

        for (auto & s : scalars)
        {
            if (s->isValid())
                validScalars.emplace(s->name(), std::move(s));
        }
    }

    return validScalars;
}

const QMap<QString, ColorMappingRegistry::MappingCreator> & ColorMappingRegistry::mappingCreators() const
{
    return m_mappingCreators;
}

bool ColorMappingRegistry::registerImplementation(const QString & name, const MappingCreator & creator)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
        return false;

    m_mappingCreators.insert(name, creator);
    return true;
}
