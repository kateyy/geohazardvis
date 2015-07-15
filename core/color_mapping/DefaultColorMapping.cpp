#include <core/color_mapping/DefaultColorMapping.h>

#include <QString>

#include <vtkMapper.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/color_mapping/ColorMappingRegistry.h>


const QString DefaultColorMapping::s_name = "user-defined color";

const bool DefaultColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<DefaultColorMapping>);

DefaultColorMapping::DefaultColorMapping(const QList<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
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

QString DefaultColorMapping::name() const
{
    return s_name;
}

void DefaultColorMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);

    if (auto m = vtkMapper::SafeDownCast(mapper))
        m->ScalarVisibilityOff();
}

QMap<int, QPair<double, double>> DefaultColorMapping::updateBounds()
{
    return{ { 0, { 0.0, 0.0 } } };
}
