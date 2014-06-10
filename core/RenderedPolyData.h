#pragma once

#include "RenderedData.h"


class QImage;

class vtkPolyDataMapper;

class PolyDataObject;


class RenderedPolyData : public RenderedData
{
public:
    RenderedPolyData(PolyDataObject * dataObject);
    ~RenderedPolyData() override;

    PolyDataObject * polyDataObject();
    const PolyDataObject * polyDataObject() const;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;

    void updateScalarToColorMapping() override;

private:
    vtkPolyDataMapper * createDataMapper() const;
};
