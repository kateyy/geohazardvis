#include <core/canvas_export/CanvasExporterImages.h>


class CanvasExporterTIFF : public CanvasExporterImages
{
public:
    CanvasExporterTIFF();

protected:
    reflectionzeug::PropertyGroup * createPropertyGroup() override;
    QStringList fileFormats() const override;
};
