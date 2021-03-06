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

#include <functional>
#include <memory>

#include <QMap>
#include <QString>

#include <core/core_api.h>


class QStringList;
class CanvasExporter;


class CORE_API CanvasExporterRegistry
{
public:
    static const QStringList & supportedFormatNames();
    static std::unique_ptr<CanvasExporter> createExporter(const QString & formatName);

    using ExporterConstructor = std::function<std::unique_ptr<CanvasExporter>()>;
    static bool registerImplementation(const ExporterConstructor & ctor);

private:
    CanvasExporterRegistry();
    ~CanvasExporterRegistry();
    static CanvasExporterRegistry & instance();

private:
    QMap<QString, ExporterConstructor> m_formatToExporter;
    QStringList m_formatNames;
};
