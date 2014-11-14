#pragma once

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkActor;
class vtkPropCollection;
class vtkScalarsToColors;

namespace reflectionzeug
{
    class PropertyGroup;
}

class DataObject;
class ScalarsForColorMapping;


enum class ContentType
{
    Rendered3D,
    Rendered2D,
    Context2D,
    invalid
};


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

    DataObject * dataObject();
    const DataObject * dataObject() const;

    /** VTK view props visualizing the data set object and possibly additional attributes */
    vtkSmartPointer<vtkPropCollection> viewProps();

    bool isVisible() const;
    void setVisible(bool visible);

    virtual reflectionzeug::PropertyGroup * createConfigGroup() = 0;

    /** set scalars that will configure color mapping for this data */
    void setScalarsForColorMapping(ScalarsForColorMapping * scalars);
    /** Set gradient that will be applied to colored geometries.
      * ScalarsForColorMapping are responsible for gradient configuration. */
    void setColorMappingGradient(vtkScalarsToColors * gradient);

signals:
    void visibilityChanged(bool visible);
    void geometryChanged();
    void viewPropCollectionChanged();

protected:
    virtual vtkSmartPointer<vtkPropCollection> fetchViewProps() = 0;
    void invalidateViewProps();

    virtual void scalarsForColorMappingChangedEvent();
    virtual void colorMappingGradientChangedEvent();
    virtual void visibilityChangedEvent(bool visible);

private:
    DataObject * m_dataObject;

protected:
    ScalarsForColorMapping * m_scalars;
    vtkSmartPointer<vtkScalarsToColors> m_gradient;

private:
    vtkSmartPointer<vtkPropCollection> m_viewProps;
    bool m_viewPropsInvalid;

    bool m_isVisible;
};
