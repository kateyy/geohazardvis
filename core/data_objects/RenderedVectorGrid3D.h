#pragma once

#include <core/data_objects/RenderedData.h>


class vtkLineSource;
class vtkGlyph3DMapper;

class VectorGrid3DDataObject;


class CORE_API RenderedVectorGrid3D : public RenderedData
{
public:
    RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject);
    ~RenderedVectorGrid3D() override;

    VectorGrid3DDataObject * vectorGrid3DDataObject();
    const VectorGrid3DDataObject * vectorGrid3DDataObject() const;

    reflectionzeug::PropertyGroup * createConfigGroup() override;

protected:
    vtkProperty * createDefaultRenderProperty() const override;
    vtkActor * createActor() override;

private:
    vtkSmartPointer<vtkLineSource> m_lineSource;
    vtkSmartPointer<vtkGlyph3DMapper> m_glyphMapper;
};
