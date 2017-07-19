/*
 * GeohazardVis
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

#include <memory>
#include <type_traits>

#include <core/core_api.h>


class QString;
class DataObject;


class CORE_API Loader
{
public:
    static std::unique_ptr<DataObject> readFile(const QString & filename);

    template<typename T> 
    static typename std::enable_if<std::is_base_of<DataObject, T>::value, std::unique_ptr<T>>::type
    readFile(const QString & fileName);

private:
    Loader() = delete;
    ~Loader() = delete;
};


#include "Loader.hpp"
