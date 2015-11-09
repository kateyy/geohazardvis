#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <QMap>

#include <core/core_api.h>


class RenderedData;
class GlyphMappingData;


class CORE_API GlyphMappingRegistry
{
public:
    static GlyphMappingRegistry & instance();

    using MappingCreator = std::function<std::vector<std::unique_ptr<GlyphMappingData>>(RenderedData & renderedData)>;
    bool registerImplementation(const QString & name, const MappingCreator & creator);

    /** retrieve a list of vectors that are applicable for the rendered data object */
    std::map<QString, std::unique_ptr<GlyphMappingData>> createMappingsValidFor(RenderedData & renderedData) const;

private:
    GlyphMappingRegistry();
    ~GlyphMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators() const;

    QMap<QString, MappingCreator> m_mappingCreators;
};
