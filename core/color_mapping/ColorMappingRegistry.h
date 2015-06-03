#pragma once

#include <functional>

#include <QList>
#include <QMap>

#include <core/core_api.h>


class QString;

class AbstractVisualizedData;
class ColorMappingData;


class CORE_API ColorMappingRegistry
{
public:
    static ColorMappingRegistry & instance();

    using MappingCreator = std::function<QList<ColorMappingData *>(const QList<AbstractVisualizedData*> & visualizedData)>;
    bool registerImplementation(const QString & name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
    QMap<QString, ColorMappingData *> createMappingsValidFor(const QList<AbstractVisualizedData*> & visualizedData);

private:
    ColorMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
