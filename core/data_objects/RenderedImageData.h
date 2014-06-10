#include "RenderedData.h"


class ImageDataObject;


class RenderedImageData : public RenderedData
{
public:
    RenderedImageData(ImageDataObject * dataObject);

    const ImageDataObject * imageDataObject() const;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;
};
