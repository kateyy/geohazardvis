#pragma once

#include <functional>

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class RenderedData;
class GlyphMappingData;


class CORE_API GlyphMappingRegistry
{
public:
    static GlyphMappingRegistry & instance();

    using MappingCreator = std::function<QList<GlyphMappingData *> (RenderedData * renderedData)>;
    bool registerImplementation(QString name, const MappingCreator & creator);

    /** retrieve a list of vectors that are applicable for the rendered data object */
    QMap<QString, GlyphMappingData *> createMappingsValidFor(RenderedData * renderedData);

private:
    GlyphMappingRegistry();
    ~GlyphMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
