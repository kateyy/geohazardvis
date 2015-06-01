#include "CanvasExporterImages.h"

#include <limits>

#include <QFileInfo>

#include <vtkImageShiftScale.h>
#include <vtkImageWriter.h>
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

CanvasExporterImages::CanvasExporterImages(vtkImageWriter * writer)
    : CanvasExporter()
    , m_toImageFilter(vtkSmartPointer<vtkWindowToImageFilter>::New())
    , m_writer(vtkSmartPointer<vtkImageWriter>::Take(writer))
{
    m_toImageFilter->SetInput(renderWindow());
    m_toImageFilter->SetInputBufferTypeToRGBA();
    //m_toImageFilter->ReadFrontBufferOff();

    m_depthToUChar = vtkSmartPointer<vtkImageShiftScale>::New();
    m_depthToUChar->SetOutputScalarTypeToUnsignedChar();
    m_depthToUChar->SetInputConnection(m_toImageFilter->GetOutputPort());

    m_writer->SetInputConnection(m_depthToUChar->GetOutputPort());
}

bool CanvasExporterImages::write()
{
    m_toImageFilter->SetInput(nullptr); // the filter does not update its image after one successful export without this line
    m_toImageFilter->SetInput(renderWindow());

    double valueScale = m_toImageFilter->GetInputBufferType() == VTK_ZBUFFER
        ? (double)std::numeric_limits<unsigned char>::max()
        : 1.0;

    m_depthToUChar->SetScale(valueScale);

    m_writer->SetFileName(verifiedFileName().toUtf8().data());
    m_writer->Write();

    renderWindow()->Render();

    return true;
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
