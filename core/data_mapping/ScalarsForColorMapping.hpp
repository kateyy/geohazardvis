#pragma once


template<typename SubClass>
ScalarsForColorMapping * ScalarsForColorMapping::newInstance(const QList<DataObject*> & dataObjects)
{
    return new SubClass(dataObjects);
}
