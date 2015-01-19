#pragma once

#include <functional>

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class AbstractVisualizedData;
class ColorMappingData;


class CORE_API ColorMappingRegistry
{
public:
    static ColorMappingRegistry & instance();

    using MappingCreator = std::function<QList<ColorMappingData *>(const QList<AbstractVisualizedData*> & visualizedData)>;
    bool registerImplementation(QString name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
    QMap<QString, ColorMappingData *> createMappingsValidFor(const QList<AbstractVisualizedData*> & visualizedData);

private:
    ColorMappingRegistry();
    ~ColorMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
