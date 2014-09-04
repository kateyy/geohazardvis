#pragma once

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkPolyData;
class vtkMapper;
class vtkActor;
class vtkArrowSource;
class vtkGlyph3D;

namespace reflectionzeug {
    class PropertyGroup;
}


enum class NormalType
{
    CellNormal,
    PointNormal
};

class CORE_API NormalRepresentation : public QObject
{
    Q_OBJECT

public:
    NormalRepresentation();

    void setData(vtkPolyData * geometry);

    vtkActor * actor();

    bool visible() const;
    void setVisible(bool visible);

    float arrowLength() const;
    void setArrowLength(float length);

    float arrowRadius() const;
    void setArrowRadius(float radius);

    float arrowTipLength() const;
    void setArrowTipLength(float tipLength);

    reflectionzeug::PropertyGroup * createPropertyGroup();

signals:
    void geometryChanged();

private:
    void updateGlyphs();

private:
    bool m_visible;
    bool m_showDispVecs;
    NormalType m_normalType;
    bool m_normalTypeChanged;
    vtkSmartPointer<vtkPolyData> m_polyData;
    bool m_polyDataChanged;
    vtkSmartPointer<vtkActor> m_actor;
    vtkSmartPointer<vtkMapper> m_mapper;

    vtkSmartPointer<vtkArrowSource> m_arrowSource;
    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
};
