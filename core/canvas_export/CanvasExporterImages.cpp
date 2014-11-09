#include "CanvasExporterImages.h"

#include <QFileInfo>

#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>

#include <reflectionzeug/PropertyGroup.h>


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

CanvasExporterImages::CanvasExporterImages()
    : CanvasExporter()
    , m_toImageFilter(vtkSmartPointer<vtkWindowToImageFilter>::New())
{
    m_toImageFilter->SetInput(renderWindow());
    m_toImageFilter->SetInputBufferTypeToRGBA();
    //m_toImageFilter->ReadFrontBufferOff();
}

PropertyGroup * CanvasExporterImages::createPropertyGroup()
{
    auto group = new PropertyGroup();

    auto prop_bufferType = group->addProperty<BufferType>("BufferType",
        [this] () { return static_cast<BufferType>(m_toImageFilter->GetInputBufferType()); },
        [this] (BufferType type) { m_toImageFilter->SetInputBufferType(static_cast<int>(type)); });
    prop_bufferType->setOption("title", "Buffer Type");
    prop_bufferType->setStrings({
            { rgb, "Colors (RGB)" },
            { rgba, "Colors and Alpha Channel (RGBA)" },
            { zBuffer, "Depth Buffer" } }
    );

    auto prop_magnification = group->addProperty<int>("Magnification",
        [this] () { return m_toImageFilter->GetMagnification(); },
        [this] (int value) { m_toImageFilter->SetMagnification(value); }
    );
    prop_magnification->setOption("minimum", m_toImageFilter->GetMagnificationMinValue());
    prop_magnification->setOption("maximum", m_toImageFilter->GetMagnificationMaxValue());

    return group;
}

QString CanvasExporterImages::verifiedFileName()
{
    QString ext = outputFormat().toLower();
    QFileInfo info(outputFileName());
    if (info.suffix().toLower() == ext)
        return outputFileName();
    else
        return outputFileName() + "." + ext;
}
