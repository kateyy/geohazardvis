#pragma once


template<typename SubClass>
ScalarsForColorMapping * ScalarsForColorMapping::newInstance(const QList<DataObject*> & dataObjects)
{
    ScalarsForColorMapping * mapping = new SubClass(dataObjects);
    mapping->initialize();

    return mapping;
}
