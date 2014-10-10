#include "DefaultColorMapping.h"

#include <vtkMapper.h>

#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>
#include <core/data_objects/DataObject.h>


const QString DefaultColorMapping::s_name = "user-defined color";

const bool DefaultColorMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<DefaultColorMapping>);

DefaultColorMapping::DefaultColorMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
{
    // only makes sense if there is a surface which we can color
    for (DataObject * dataObject : dataObjects)
    {
        if (dataObject->is3D())
        {
            m_valid = true;
            break;
        }
    }
}

DefaultColorMapping::~DefaultColorMapping() = default;

QString DefaultColorMapping::name() const
{
    return s_name;
}

void DefaultColorMapping::configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper)
{
    ScalarsForColorMapping::configureDataObjectAndMapper(dataObject, mapper);

    mapper->ScalarVisibilityOff();
}

void DefaultColorMapping::updateBounds()
{
    setDataMinMaxValue(0.0, 0.0, 0);
}

bool DefaultColorMapping::isValid() const
{
    return m_valid;
}
