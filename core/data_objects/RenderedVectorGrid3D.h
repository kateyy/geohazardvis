#pragma once

#include <core/data_objects/RenderedData.h>


class vtkGlyph3D;

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
    vtkSmartPointer<vtkGlyph3D> m_glyph;
};
