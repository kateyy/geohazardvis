#include "DefaultColorMapping.h"

#include <vtkMapper.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


const QString DefaultColorMapping::s_name = "user-defined color";

const bool DefaultColorMapping::s_isRegistered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<DefaultColorMapping>);

DefaultColorMapping::DefaultColorMapping(const QList<AbstractVisualizedData *> & visualizedData)
    : ScalarsForColorMapping(visualizedData)
{
    // only makes sense if there is a surface which we can color
    for (AbstractVisualizedData * vis : visualizedData)
    {
        if (vis->contentType() == ContentType::Rendered3D)
        {
            m_isValid = true;
            break;
        }
    }
}

DefaultColorMapping::~DefaultColorMapping() = default;

QString DefaultColorMapping::name() const
{
    return s_name;
}

void DefaultColorMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper)
{
    ScalarsForColorMapping::configureMapper(visualizedData, mapper);

    mapper->ScalarVisibilityOff();
}

void DefaultColorMapping::updateBounds()
{
    setDataMinMaxValue(0.0, 0.0, 0);
}
