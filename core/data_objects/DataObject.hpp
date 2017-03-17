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
    : ScopedEventDeferral(&objectToLock)
{
}

inline ScopedEventDeferral::ScopedEventDeferral(DataObject * objectToLock)
    : m_dataObject{ objectToLock }
{
    if (m_dataObject)
    {
        m_dataObject->deferEvents();
    }
}

inline ScopedEventDeferral::~ScopedEventDeferral()
{
    if (m_dataObject)
    {
        m_dataObject->executeDeferredEvents();
    }
}

inline DataObject * ScopedEventDeferral::lockedObject()
{
    return m_dataObject;
}

inline ScopedEventDeferral::ScopedEventDeferral(ScopedEventDeferral && other)
    : m_dataObject{ nullptr }
{
    std::swap(m_dataObject, other.m_dataObject);
}

inline ScopedEventDeferral & ScopedEventDeferral::operator=(ScopedEventDeferral && other)
{
    std::swap(m_dataObject, other.m_dataObject);

    return *this;
}
