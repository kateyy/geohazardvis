#pragma once

#include <QObject>

#include <vtkSmartPointer.h>


class vtkPolyData;
class vtkMapper;
class vtkActor;
class vtkGlyph3D;

namespace reflectionzeug {
    class PropertyGroup;
}


class NormalRepresentation : public QObject
{
    Q_OBJECT

public:
    NormalRepresentation();

    void setData(vtkPolyData * geometry);

    vtkActor * actor();

    void setVisible(bool visible);
    bool visible() const;

    void setGlyphSize(float size);
    float glyphSize() const;

    reflectionzeug::PropertyGroup * createPropertyGroup();

signals:
    void geometryChanged();

private:
    void updateGlyphs();

private:
    bool m_visible;
    vtkSmartPointer<vtkPolyData> m_polyData;
    bool m_polyDataChanged;
    vtkSmartPointer<vtkActor> m_actor;
    vtkSmartPointer<vtkMapper> m_mapper;

    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
};
