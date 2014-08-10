#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/RenderedData.h>
#include <core/NormalRepresentation.h>
#include <core/core_api.h>


class QImage;

class vtkPolyDataMapper;
class vtkAlgorithm;

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

    void updateScalarToColorMapping() override;

private:
    vtkPolyDataMapper * createDataMapper();

    NormalRepresentation m_normalRepresentation;

    vtkSmartPointer<vtkAlgorithm> m_filter;
};
