#include <core/color_mapping/DefaultColorMapping.h>

#include <QString>

#include <vtkMapper.h>

#include <core/types.h>
#include <core/utility/DataExtent.h>


const QString DefaultColorMapping::s_name = "user-defined color";

DefaultColorMapping::DefaultColorMapping(const std::vector<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
{
    m_isValid = true;
}

DefaultColorMapping::~DefaultColorMapping() = default;

QString DefaultColorMapping::name() const
{
    return s_name;
}

void DefaultColorMapping::configureMapper(
    AbstractVisualizedData & visualizedData,
    vtkAbstractMapper & mapper,
    unsigned int port)
{
    ColorMappingData::configureMapper(visualizedData, mapper, port);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->ScalarVisibilityOff();
    }
}

std::vector<ValueRange<>> DefaultColorMapping::updateBounds()
{
    return{ ValueRange<>({ 0.0, 0.0 }) };
}
