/*
 * GeohazardVis plug-in: pCDM Modeling
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
