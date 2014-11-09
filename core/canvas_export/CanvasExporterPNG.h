#include <core/canvas_export/CanvasExporterImages.h>


class CanvasExporterPNG : public CanvasExporterImages
{
public:
    CanvasExporterPNG();

protected:
    reflectionzeug::PropertyGroup * createPropertyGroup() override;
    QStringList fileFormats() const override;
};
