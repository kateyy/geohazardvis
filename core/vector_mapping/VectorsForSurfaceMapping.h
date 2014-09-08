#pragma once

#include <QList>
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

public:
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
    template<typename SubClass>
    /** default function for Registry::MappingCreator, returning a single mapping instance */
    static QList<VectorsForSurfaceMapping *> newInstance(RenderedData * renderedData);
    explicit VectorsForSurfaceMapping(RenderedData * renderedData);

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
