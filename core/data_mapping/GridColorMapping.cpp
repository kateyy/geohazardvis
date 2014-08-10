#include "GridColorMapping.h"

#include "ScalarsForColorMappingRegistry.h"

#include <core/data_objects/ImageDataObject.h>


const QString GridColorMapping::s_name = "grid value";

const bool GridColorMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<GridColorMapping>);

GridColorMapping::GridColorMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
    , m_isValid(dataObjects.size() == 1 && dynamic_cast<ImageDataObject*>(dataObjects.first()))
{
}

GridColorMapping::~GridColorMapping() = default;

QString GridColorMapping::name() const
{
    return s_name;
}

bool GridColorMapping::usesGradients() const
{
    return true;
}

void GridColorMapping::updateBounds()
{
    m_dataMinValue = 0;
    m_dataMaxValue = 0;
    ScalarsForColorMapping::updateBounds();
}

bool GridColorMapping::isValid() const
{
    return m_isValid;
}
