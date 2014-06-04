#include "RenderedData.h"


class ImageDataObject;


class RenderedImageData : public RenderedData
{
public:
    RenderedImageData(std::shared_ptr<const ImageDataObject> dataObject);

    std::shared_ptr<const ImageDataObject> imageDataObject() const;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;
};
