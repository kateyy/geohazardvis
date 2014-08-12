#pragma once

#include <core/data_objects/RenderedData.h>
#include <core/NormalRepresentation.h>
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

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() override;
    QList<vtkActor *> fetchAttributeActors() override;

    void scalarsForColorMappingChangedEvent() override;
    void gradientForColorMappingChangedEvent() override;

private:
    vtkPolyDataMapper * createDataMapper();

    NormalRepresentation m_normalRepresentation;
};
