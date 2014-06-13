#pragma once

#include <core/data_objects/RenderedData.h>
#include <core/core_api.h>


class QImage;

class vtkPolyDataMapper;

class PolyDataObject;


class CORE_API RenderedPolyData : public RenderedData
{
public:
    RenderedPolyData(PolyDataObject * dataObject);
    ~RenderedPolyData() override;

    PolyDataObject * polyDataObject();
    const PolyDataObject * polyDataObject() const;

    reflectionzeug::PropertyGroup * configGroup();

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() const override;

    void updateScalarToColorMapping() override;

private:
    vtkPolyDataMapper * createDataMapper() const;

    reflectionzeug::PropertyGroup * m_configGroup;
};
