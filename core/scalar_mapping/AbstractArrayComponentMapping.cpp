#include "AbstractArrayComponentMapping.h"

#include <cassert>


AbstractArrayComponentMapping::AbstractArrayComponentMapping(const QList<DataObject *> & dataObjects, QString dataArrayName, vtkIdType component)
    : ScalarsForColorMapping(dataObjects)
    , m_isValid(false)
    , m_dataArrayName(dataArrayName)
    , m_component(component)
    , m_arrayNumComponents(0)
{
}

QString AbstractArrayComponentMapping::name() const
{
    assert(m_arrayNumComponents);   // subclasses must set this value

    if (m_arrayNumComponents == 1)
        return m_dataArrayName;

    QString component = m_arrayNumComponents <= 3
        ? QChar::fromLatin1('x' + m_component)
        : QString::number(m_component);

    return m_dataArrayName + " (" + component + ")";
}

bool AbstractArrayComponentMapping::isValid() const
{
    return m_isValid;
}
