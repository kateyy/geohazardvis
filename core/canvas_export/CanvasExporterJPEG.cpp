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

#include "CanvasExporterJPEG.h"

#include <QStringList>

#include <vtkJPEGWriter.h>
#include <vtkNew.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/canvas_export/CanvasExporterRegistry.h>


const bool CanvasExporterJPEG::s_isRegistered = CanvasExporterRegistry::registerImplementation(CanvasExporter::newInstance<CanvasExporterJPEG>);


CanvasExporterJPEG::CanvasExporterJPEG()
    : CanvasExporterImages(vtkNew<vtkJPEGWriter>().Get())
{
}

CanvasExporterJPEG::~CanvasExporterJPEG() = default;

std::unique_ptr<reflectionzeug::PropertyGroup> CanvasExporterJPEG::createPropertyGroup()
{
    auto group = CanvasExporterImages::createPropertyGroup();

    auto jpgWriter = static_cast<vtkJPEGWriter *>(m_writer.Get());

    group->addProperty<int>("Quality",
        [jpgWriter] () { return jpgWriter->GetQuality(); },
        [jpgWriter] (int q) { jpgWriter->SetQuality(q); })
        ->setOptions({
            { "minimum", jpgWriter->GetQualityMinValue() },
            { "maximum", jpgWriter->GetQualityMaxValue() }
    });

    group->addProperty<bool>("Progressive",
        [jpgWriter] () { return jpgWriter->GetProgressive() != 0; },
        [jpgWriter] (bool value) { jpgWriter->SetProgressive(value); });

    return group;
}

QStringList CanvasExporterJPEG::fileFormats() const
{
    return{ "JPEG" };
}
