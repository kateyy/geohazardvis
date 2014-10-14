#pragma once

#include <functional>

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class RenderedData;
class VectorMappingData;


class CORE_API VectorMappingRegistry
{
public:
    static VectorMappingRegistry & instance();

    using MappingCreator = std::function<QList<VectorMappingData *> (RenderedData * renderedData)>;
    bool registerImplementation(QString name, const MappingCreator & creator);

    /** retrieve a list of vectors that are applicable for the rendered data object */
    QMap<QString, VectorMappingData *> createMappingsValidFor(RenderedData * renderedData);

private:
    VectorMappingRegistry();
    ~VectorMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
