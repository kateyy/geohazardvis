#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkActor;
class vtkPolyData;
class vtkMapper;
class vtkArrowSource;
class vtkGlyph3D;

namespace reflectionzeug
{
    class PropertyGroup;
}

class RenderedData;


/**
Abstract base class for vectors that can be mapped to polygonal surfaces.
*/
class CORE_API VectorsForSurfaceMapping : public QObject
{
    Q_OBJECT

public:
    friend class VectorsForSurfaceMappingRegistry;

    template<typename SubClass>
    static VectorsForSurfaceMapping * newInstance(RenderedData * renderedData);

public:
    explicit VectorsForSurfaceMapping(RenderedData * renderedData);
    virtual ~VectorsForSurfaceMapping() = 0;

    virtual QString name() const = 0;

    bool isVisible() const;
    void setVisible(bool enabled);

    float arrowLength() const;
    void setArrowLength(float length);

    float arrowRadius() const;
    void setArrowRadius(float radius);

    float arrowTipLength() const;
    void setArrowTipLength(float tipLength);

    vtkActor * actor();

    virtual reflectionzeug::PropertyGroup * createPropertyGroup();

signals:
    void geometryChanged();

protected:
    virtual void initialize();

    virtual bool isValid() const;

    RenderedData * renderedData();
    vtkPolyData * polyData();
    vtkGlyph3D * arrowGlyph();

    virtual void visibilityChangedEvent();

private:
    RenderedData * m_renderedData;

    bool m_isVisible;

    vtkSmartPointer<vtkArrowSource> m_arrowSource;
    vtkSmartPointer<vtkPolyData> m_polyData;
    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
    vtkSmartPointer<vtkMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;

};

#include "VectorsForSurfaceMapping.hpp"
