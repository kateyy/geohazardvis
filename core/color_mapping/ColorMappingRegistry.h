#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QMap>

#include <core/core_api.h>


class QString;
template<typename T> class QList;

class AbstractVisualizedData;
class ColorMappingData;


class CORE_API ColorMappingRegistry
{
public:
    static ColorMappingRegistry & instance();

    using MappingCreator = std::function<std::vector<std::unique_ptr<ColorMappingData>>(const QList<AbstractVisualizedData*> & visualizedData)>;
    bool registerImplementation(const QString & name, const MappingCreator & creator);

    /** retrieve a list of scalars extractions that are applicable for the specified data object list */
    std::map<QString, std::unique_ptr<ColorMappingData>> createMappingsValidFor(const QList<AbstractVisualizedData*> & visualizedData);

private:
    ColorMappingRegistry();
    ~ColorMappingRegistry();

    const QMap<QString, MappingCreator> & mappingCreators();

    QMap<QString, MappingCreator> m_mappingCreators;
};
