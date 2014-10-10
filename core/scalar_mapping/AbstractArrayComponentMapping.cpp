#include "AbstractArrayComponentMapping.h"

#include <cassert>


AbstractArrayComponentMapping::AbstractArrayComponentMapping(const QList<DataObject *> & dataObjects, QString dataArrayName, vtkIdType numDataComponents)
    : ScalarsForColorMapping(dataObjects, numDataComponents)
    , m_isValid(false)
    , m_dataArrayName(dataArrayName)
{
}

QString AbstractArrayComponentMapping::name() const
{
    return m_dataArrayName;
}

bool AbstractArrayComponentMapping::isValid() const
{
    return m_isValid;
}
