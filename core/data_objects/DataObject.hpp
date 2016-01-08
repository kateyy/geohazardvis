#pragma once

#include "DataObject.h"


template<typename U, typename T>
void DataObject::connectObserver(const QString & eventName, vtkObject & subject, unsigned long event, U & observer, void(T::* slot)(void))
{
    addObserver(eventName, subject,
        subject.AddObserver(event, &observer, slot));
}

template<typename U, typename T>
void DataObject::connectObserver(const QString & eventName, vtkObject & subject, unsigned long event, U & observer, void(T::* slot)(vtkObject*, unsigned long, void*))
{
    addObserver(eventName, subject,
        subject.AddObserver(event, &observer, slot));
}


inline ScopedEventDeferral::ScopedEventDeferral(DataObject & objectToLock)
    : m_dataObject(objectToLock)
{
    m_dataObject.deferEvents();
}

inline ScopedEventDeferral::~ScopedEventDeferral()
{
    m_dataObject.executeDeferredEvents();
}
