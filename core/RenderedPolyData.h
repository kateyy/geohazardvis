#pragma once

#include <memory>

#include "RenderedData.h"


class QImage;

class vtkPolyDataMapper;

class PolyDataObject;
enum class DataSelection;


class RenderedPolyData : public RenderedData
{
public:
    RenderedPolyData(std::shared_ptr<const PolyDataObject> dataObject);
    ~RenderedPolyData() override;

    std::shared_ptr<const PolyDataObject> polyDataObject() const;

    void setSurfaceColorMapping(DataSelection dataSelection, const QImage & gradient);

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;

private:
    vtkPolyDataMapper * createDataMapper(DataSelection dataSelection, const QImage & gradient) const;
};
