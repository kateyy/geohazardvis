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

#include "CanvasExporterImages.h"

#include <limits>

#include <QFileInfo>

#include <vtkImageShiftScale.h>
#include <vtkImageWriter.h>
#include <vtkRenderWindow.h>
#include <vtkVersionMacros.h>
#include <vtkWindowToImageFilter.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/utility/macros.h>


using namespace reflectionzeug;

namespace
{
enum BufferType
{
    rgb = VTK_RGB,
    rgba = VTK_RGBA,
    zBuffer = VTK_ZBUFFER
};
}

CanvasExporterImages::CanvasExporterImages(vtkImageWriter * writer)
    : CanvasExporter()
    , m_toImageFilter{ vtkSmartPointer<vtkWindowToImageFilter>::New() }
    , m_writer{ writer }
{
    m_toImageFilter->SetInput(renderWindow());
    m_toImageFilter->SetInputBufferTypeToRGBA();
    m_toImageFilter->ReadFrontBufferOff();

    m_depthToUChar = vtkSmartPointer<vtkImageShiftScale>::New();
    m_depthToUChar->SetOutputScalarTypeToUnsignedChar();
    m_depthToUChar->SetInputConnection(m_toImageFilter->GetOutputPort());

    m_writer->SetInputConnection(m_depthToUChar->GetOutputPort());
}

CanvasExporterImages::~CanvasExporterImages() = default;

bool CanvasExporterImages::write()
{
    m_toImageFilter->SetInput(nullptr); // the filter does not update its image after one successful export without this line
    m_toImageFilter->SetInput(renderWindow());

    const auto previousSwapBuffers = renderWindow()->GetSwapBuffers();
    renderWindow()->SwapBuffersOff();

    double valueScale = m_toImageFilter->GetInputBufferType() == VTK_ZBUFFER
        ? (double)std::numeric_limits<unsigned char>::max()
        : 1.0;

    m_depthToUChar->SetScale(valueScale);

    m_writer->SetFileName(verifiedFileName().toUtf8().data());
    m_writer->Write();

    renderWindow()->SetSwapBuffers(previousSwapBuffers);

    return true;
}

std::unique_ptr<PropertyGroup> CanvasExporterImages::createPropertyGroup()
{
    auto group = std::make_unique<PropertyGroup>();

    auto prop_bufferType = group->addProperty<BufferType>("BufferType",
        [this] () { return static_cast<BufferType>(m_toImageFilter->GetInputBufferType()); },
        [this] (BufferType type) { m_toImageFilter->SetInputBufferType(static_cast<int>(type)); });
    prop_bufferType->setOption("title", "Buffer Type");
    prop_bufferType->setStrings({
            { rgb, "Colors (RGB)" },
            { rgba, "Colors and Alpha Channel (RGBA)" },
            { zBuffer, "Depth Buffer" } }
    );

#if VTK_CHECK_VERSION(8,1,0)
    group->addProperty<int>("Magnification",
        [this] () { return m_toImageFilter->GetScale()[0]; },
        [this] (int value) { m_toImageFilter->SetScale(value); })
        ->setOptions({
            { "minimum", 1 },
            { "maximum", 100000 }
        });
#else
    auto prop_magnification = group->addProperty<int>("Magnification",
        [this] () { return m_toImageFilter->GetMagnification(); },
        [this] (int value) { m_toImageFilter->SetMagnification(value); })
        ->setOptions({
            { "minimum", m_toImageFilter->GetMagnificationMinValue() },
            { "maximum", m_toImageFilter->GetMagnificationMaxValue() }
        });
#endif

    return group;
}

QString CanvasExporterImages::verifiedFileName() const
{
    QString ext = outputFormat().toLower();
    QFileInfo info(outputFileName());
    if (info.suffix().toLower() == ext)
    {
        return outputFileName();
    }
    return outputFileName() + "." + ext;
}
