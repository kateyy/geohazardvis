#include <core/canvas_export/CanvasExporterImages.h>


class vtkPNGWriter;


class CanvasExporterPNG : public CanvasExporterImages
{
public:
    CanvasExporterPNG();

    bool write() override;

protected:
    reflectionzeug::PropertyGroup * createPropertyGroup() override;
    QStringList fileFormats() const override;

private:
    vtkSmartPointer<vtkPNGWriter> m_writer;
};
