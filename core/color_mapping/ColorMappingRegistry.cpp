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

std::map<QString, std::unique_ptr<ColorMappingData>> ColorMappingRegistry::createMappingsValidFor(
    const std::vector<AbstractVisualizedData *> & visualizedData) const
{
    std::map<QString, std::unique_ptr<ColorMappingData>> validScalars;

    for (const auto & creator : m_mappingCreators)
    {
        auto scalars = creator.second(visualizedData);

        for (auto & s : scalars)
        {
            if (s->isValid())
            {
                validScalars.emplace(s->name(), std::move(s));
            }
        }
    }

    return validScalars;
}

bool ColorMappingRegistry::registerImplementation(const QString & name, const MappingCreator & creator)
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
