#pragma once

#include <functional>

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class AbstractVisualizedData;
class ScalarsForColorMapping;


class CORE_API ScalarsForColorMappingRegistry
{
public:
    static ScalarsForColorMappingRegistry & instance();

    using MappingCreator = std::function<QList<ScalarsForColorMapping *>(const QList<AbstractVisualizedData*> & visualizedData)>;
    bool registerImplementation(QString name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
    QMap<QString, ScalarsForColorMapping *> createMappingsValidFor(const QList<AbstractVisualizedData*> & visualizedData);

private:
    ScalarsForColorMappingRegistry();
    ~ScalarsForColorMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
