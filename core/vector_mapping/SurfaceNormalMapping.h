#pragma once

#include <vtkSmartPointer.h>

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkPolyData;
class vtkMapper;
class vtkActor;
class vtkArrowSource;
class vtkGlyph3D;

namespace reflectionzeug
{
    class PropertyGroup;
}


enum class NormalType
{
    CellNormal,
    PointNormal
};

class CORE_API SurfaceNormalMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    SurfaceNormalMapping(RenderedData * renderedData);
    ~SurfaceNormalMapping() override;

    QString name() const override;

    float arrowLength() const;
    void setArrowLength(float length);

    float arrowRadius() const;
    void setArrowRadius(float radius);

    float arrowTipLength() const;
    void setArrowTipLength(float tipLength);

    reflectionzeug::PropertyGroup * createPropertyGroup() override;

signals:
    void geometryChanged();

protected:
    bool isValid() const override;
    void visibilityChangedEvent() override;

private:
    void updateGlyphs();

private:
    NormalType m_normalType;
    bool m_normalTypeChanged;
    vtkSmartPointer<vtkPolyData> m_polyData;
    vtkSmartPointer<vtkMapper> m_mapper;

    vtkSmartPointer<vtkArrowSource> m_arrowSource;
    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;

    static const bool s_registered;
};
