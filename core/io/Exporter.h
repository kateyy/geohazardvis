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

#include <core/core_api.h>


template <class Key, class T> class QMap;
class QString;
class DataObject;
class ImageDataObject;


class CORE_API Exporter
{
public:
    static bool exportData(DataObject & data, const QString & fileName);

    static bool isExportSupported(const DataObject & data);
    static QString formatFilter(const DataObject & data);
    static const QMap<QString, QString> & formatFilters();

    static bool exportImageFormat(ImageDataObject & image, const QString & fileName);
    static bool exportVTKXMLPolyData(DataObject & polyData, const QString & fileName);
    static bool exportVTKXMLImageData(DataObject & image, const QString & fileName);

private:
    Exporter() = delete;
    ~Exporter() = delete;
};
