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

#include "Loader.h"


template<typename T> typename std::enable_if<std::is_base_of<DataObject, T>::value, std::unique_ptr<T>>::type
Loader::readFile(const QString & fileName)
{
    auto genericDataSet = readFile(fileName);
    auto specificDataSet = dynamic_cast<T *>(genericDataSet.get());
    if (!specificDataSet)
    {
        return nullptr;
    }

    genericDataSet.release();

    return std::unique_ptr<T>(specificDataSet);
}
