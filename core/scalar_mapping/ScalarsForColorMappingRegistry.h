#pragma once

#include <functional>

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class DataObject;
class ScalarsForColorMapping;


class CORE_API ScalarsForColorMappingRegistry
{
public:
    static ScalarsForColorMappingRegistry & instance();

    using MappingCreator = std::function<QList<ScalarsForColorMapping *>(const QList<DataObject*> & dataObjects)>;
    bool registerImplementation(QString name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
     QMap<QString, ScalarsForColorMapping *> createMappingsValidFor(const QList<DataObject*> & dataObjects);

private:
    ScalarsForColorMappingRegistry();
    ~ScalarsForColorMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
