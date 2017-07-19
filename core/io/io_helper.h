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

#include <map>

#include <QStringList>

#include <core/core_api.h>


namespace io
{

enum Category
{
    all,
    CSV,
    PolyData,
    Image2D,
    VTKImageFormats, // subset of Image2D that is supported by VTK
    Volume,
};

/**
 * Provide a list of file extension filters as expected by QFileDialog for all supported file types
 * of the specified category. If multiple file types are supported, an entry "All Supported Files"
 * is added.
 */
CORE_API const QString & fileFormatFilters(Category category = Category::all);
CORE_API const std::map<QString, QStringList> & fileFormatExtensions(Category category = Category::all);

/** Replace characters not representable in the current OS's file system logic. */
CORE_API QString normalizeFileName(const QString & fileName, const QString & replaceString = "_");

}
