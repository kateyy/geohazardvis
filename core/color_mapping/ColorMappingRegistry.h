#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <core/core_api.h>


class QString;

class AbstractVisualizedData;
class ColorMappingData;


class CORE_API ColorMappingRegistry
{
public:
    static ColorMappingRegistry & instance();

    using MappingCreator = std::function<std::vector<std::unique_ptr<ColorMappingData>>(
        const std::vector<AbstractVisualizedData *> & visualizedData)>;
    bool registerImplementation(const QString & name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
    std::map<QString, std::unique_ptr<ColorMappingData>> createMappingsValidFor(
        const std::vector<AbstractVisualizedData*> & visualizedData) const;

private:
    ColorMappingRegistry();
    ~ColorMappingRegistry();

    std::map<QString, MappingCreator> m_mappingCreators;

private:
    ColorMappingRegistry(const ColorMappingRegistry &) = delete;
    void operator=(const ColorMappingRegistry &) = delete;
};
