#pragma once


template<typename SubClass>
ScalarsForColorMapping * ScalarsForColorMapping::newInstance()
{
    return new SubClass;
}
