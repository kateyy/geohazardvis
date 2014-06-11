#pragma once

#include <functional>

#include <QString>
#include <QMap>


class DataObject;
class ScalarsForColorMapping;


class ScalarsForColorMappingRegistry
{
public:
    static ScalarsForColorMappingRegistry & instance();

    using MappingConstructor = std::function<ScalarsForColorMapping *(const QList<DataObject *> & dataObjects)>;
    bool registerImplementation(QString name, const MappingConstructor & constructor);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
     QMap<QString, ScalarsForColorMapping *> createMappingsValidFor(const QList<DataObject*> & dataObjects);

private:
    ScalarsForColorMappingRegistry();
    ~ScalarsForColorMappingRegistry();

    const QMap<QString, MappingConstructor> & mappingConstructors();

    QMap<QString, MappingConstructor> m_mappingConstructors;
};
