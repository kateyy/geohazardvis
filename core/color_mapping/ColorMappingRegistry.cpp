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

QMap<QString, ColorMappingData *> ColorMappingRegistry::createMappingsValidFor(const QList<AbstractVisualizedData*> & visualizedData)
{
    QMap<QString, ColorMappingData *> validScalars;

    for (auto creator : m_mappingCreators)
    {
        QList<ColorMappingData *> scalars = creator(visualizedData);

        for (ColorMappingData * s : scalars)
        {
            if (s->isValid())
                validScalars.insert(s->name(), s);
            else
                delete s;
        }
    }

    return validScalars;
}

const QMap<QString, ColorMappingRegistry::MappingCreator> & ColorMappingRegistry::mappingCreators()
{
    return m_mappingCreators;
}

bool ColorMappingRegistry::registerImplementation(QString name, const MappingCreator & creator)
{
    assert(!m_mappingCreators.contains(name));
    if (m_mappingCreators.contains(name))
        return false;

    m_mappingCreators.insert(name, creator);
    return true;
}
