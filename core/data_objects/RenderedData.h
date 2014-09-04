#pragma once

#include <QObject>
#include <QList>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkLookupTable;
class vtkProperty;
class vtkActor;

namespace reflectionzeug
{
    class PropertyGroup;
}

class DataObject;
class ScalarsForColorMapping;
class VectorsToSurfaceMapping;


/**
Base class for rendered representations of loaded data objects.
A data object may be rendered in multiple views, each holding its own
RenderedData instance, referring to the data object.
*/ 
class CORE_API RenderedData : public QObject
{
    Q_OBJECT

public:
    RenderedData(DataObject * dataObject);
    virtual ~RenderedData() = 0;

    DataObject * dataObject();
    const DataObject * dataObject() const;

    vtkProperty * renderProperty();
    /** VTK actors visualizing the data object and possibly additional attributes */
    QList<vtkActor *> actors();
    vtkActor * mainActor();
    QList<vtkActor *> attributeActors();

    bool isVisible() const;
    void setVisible(bool visible);

    virtual reflectionzeug::PropertyGroup * createConfigGroup() = 0;

    void applyScalarsForColorMapping(ScalarsForColorMapping * scalars);
    void applyGradientLookupTable(vtkLookupTable * gradient);

signals:
    void geometryChanged();

protected:
    virtual vtkProperty * createDefaultRenderProperty() const = 0;
    virtual vtkActor * createActor() = 0;
    virtual QList<vtkActor *> fetchAttributeActors();

    virtual void scalarsForColorMappingChangedEvent() = 0;
    virtual void gradientForColorMappingChangedEvent() = 0;
    virtual void vectorsForSurfaceMappingChangedEvent();
    virtual void visibilityChangedEvent(bool visible);

private:
    DataObject * m_dataObject;

protected:
    ScalarsForColorMapping * m_scalars;
    vtkSmartPointer<vtkLookupTable> m_lut;
    VectorsToSurfaceMapping * m_vectors;
    unsigned long m_lutMTime;

private:
    vtkSmartPointer<vtkProperty> m_renderProperty;
    vtkSmartPointer<vtkActor> m_actor;

    bool m_isVisible;
};
