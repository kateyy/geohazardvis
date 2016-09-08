#include <core/canvas_export/CanvasExporterImages.h>


class CanvasExporterTIFF : public CanvasExporterImages
{
public:
    CanvasExporterTIFF();
    ~CanvasExporterTIFF() override;

protected:
    std::unique_ptr<reflectionzeug::PropertyGroup> createPropertyGroup() override;
    QStringList fileFormats() const override;

private:
    static const bool s_isRegistered;
};
