#pragma once

#include <functional>

#include <QString>
#include <QMap>

#include <core/core_api.h>


class RenderedData;
class VectorsForSurfaceMapping;


class CORE_API VectorsForSurfaceMappingRegistry
{
public:
    static VectorsForSurfaceMappingRegistry & instance();

    using MappingConstructor = std::function<VectorsForSurfaceMapping *(RenderedData * renderedData)>;
    bool registerImplementation(QString name, const MappingConstructor & constructor);

    /** retrieve a list of vectors that are applicable for the rendered data object */
    QMap<QString, VectorsForSurfaceMapping *> createMappingsValidFor(RenderedData * renderedData);

private:
    VectorsForSurfaceMappingRegistry();
    ~VectorsForSurfaceMappingRegistry();

    const QMap<QString, MappingConstructor> & mappingConstructors();

    QMap<QString, MappingConstructor> m_mappingConstructors;
};
