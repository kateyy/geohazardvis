#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkActor;

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

    vtkActor * actor();

    virtual reflectionzeug::PropertyGroup * createPropertyGroup() = 0;

signals:
    void geometryChanged();

protected:
    virtual void initialize();

    virtual bool isValid() const = 0;

    virtual void visibilityChangedEvent();

protected:
    RenderedData * m_renderedData;

private:
    bool m_isVisible;

    vtkSmartPointer<vtkActor> m_actor;
};

#include "VectorsForSurfaceMapping.hpp"
