#include <core/canvas_export/CanvasExporterImages.h>


class CanvasExporterJPEG : public CanvasExporterImages
{
public:
    CanvasExporterJPEG();
    ~CanvasExporterJPEG() override;

protected:
    std::unique_ptr<reflectionzeug::PropertyGroup> createPropertyGroup() override;
    QStringList fileFormats() const override;

private:
    static const bool s_isRegistered;
};
