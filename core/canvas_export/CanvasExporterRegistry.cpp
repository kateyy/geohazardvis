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

#include "CanvasExporterRegistry.h"

#include <QDebug>
#include <QStringList>

#include <core/canvas_export/CanvasExporter.h>


CanvasExporterRegistry::CanvasExporterRegistry()
{
}

CanvasExporterRegistry::~CanvasExporterRegistry() = default;

CanvasExporterRegistry & CanvasExporterRegistry::instance()
{
    static CanvasExporterRegistry reg;
    return reg;
}

const QStringList & CanvasExporterRegistry::supportedFormatNames()
{
    auto & names = instance().m_formatNames;
    if (!names.isEmpty())
    {
        return names;
    }

    names = instance().m_formatToExporter.keys();

    return names;
}

std::unique_ptr<CanvasExporter> CanvasExporterRegistry::createExporter(const QString & formatName)
{
    const ExporterConstructor & ctor = instance().m_formatToExporter.value(formatName, nullptr);
    if (!ctor)
    {
        return nullptr;
    }

    auto exporter = ctor();
    exporter->setOutputFormat(formatName);

    return exporter;
}

bool CanvasExporterRegistry::registerImplementation(const ExporterConstructor & ctor)
{
    auto exporter(ctor());

    bool okay = true;

    for (const QString & format : exporter->fileFormats())
    {
        if (instance().m_formatToExporter.contains(format))
        {
            qDebug() << "File format name already used:" << format;
            okay = false;
            continue;
        }
        instance().m_formatToExporter.insert(format, ctor);
    }

    return okay;
}
