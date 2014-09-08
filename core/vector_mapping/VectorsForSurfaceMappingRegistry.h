#pragma once

#include <functional>

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class RenderedData;
class VectorsForSurfaceMapping;


class CORE_API VectorsForSurfaceMappingRegistry
{
public:
    static VectorsForSurfaceMappingRegistry & instance();

    using MappingCreator = std::function<QList<VectorsForSurfaceMapping *> (RenderedData * renderedData)>;
    bool registerImplementation(QString name, const MappingCreator & creator);

    /** retrieve a list of vectors that are applicable for the rendered data object */
    QMap<QString, VectorsForSurfaceMapping *> createMappingsValidFor(RenderedData * renderedData);

private:
    VectorsForSurfaceMappingRegistry();
    ~VectorsForSurfaceMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
