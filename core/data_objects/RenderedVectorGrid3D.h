#pragma once

#include <array>

#include <core/data_objects/RenderedData.h>


class vtkGlyph3D;
class vtkExtractVOI;

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
    void setSampleRate(int x, int y, int z);

private:
    vtkSmartPointer<vtkGlyph3D> m_glyph;
    vtkSmartPointer<vtkExtractVOI> m_extractVOI;

    std::array<vtkSmartPointer<vtkExtractVOI>, 3> m_extractSlices;
};
