#pragma once


template<typename SubClass>
QList<ScalarsForColorMapping *> ScalarsForColorMapping::newInstance(const QList<DataObject*> & dataObjects)
{
    ScalarsForColorMapping * mapping = new SubClass(dataObjects);
    if (mapping->isValid())
        mapping->initialize();

    return{ mapping };
}
