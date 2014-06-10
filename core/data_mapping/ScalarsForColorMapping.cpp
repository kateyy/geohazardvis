#include "ScalarsForColorMapping.h"

#include <cassert>


ScalarsForColorMapping::ScalarsForColorMapping(const QList<DataObject *> & dataObjects)
{
    updateBounds();
}

ScalarsForColorMapping::~ScalarsForColorMapping() = default;

QMap<QString, ScalarsForColorMapping *> ScalarsForColorMapping::createMappingsValidFor(const QList<DataObject*> & dataObjects)
{
    QMap<QString, ScalarsForColorMapping *> validScalars;

    for (auto constr : mappingConstructors())
    {
        ScalarsForColorMapping * scalars = constr(dataObjects);

        if (scalars->isValid())
            validScalars.insert(scalars->name(), scalars);
        else
            delete scalars;
    }

    return validScalars;
}

QMap<QString, ScalarsForColorMapping::MappingConstructor> & ScalarsForColorMapping::mappingConstructors()
{
    static QMap<QString, ScalarsForColorMapping::MappingConstructor> list;

    return list;
}

bool ScalarsForColorMapping::registerImplementation(QString name, const ScalarsForColorMapping::MappingConstructor & constructor)
{
    assert(!mappingConstructors().contains(name));

    mappingConstructors().insert(name, constructor);

    return true;
}

double ScalarsForColorMapping::minValue() const
{
    assert(m_minValue <= m_maxValue);
    return m_minValue;
}

double ScalarsForColorMapping::maxValue() const
{
    assert(m_minValue <= m_maxValue);
    return m_maxValue;
}

void ScalarsForColorMapping::updateBounds()
{
}
