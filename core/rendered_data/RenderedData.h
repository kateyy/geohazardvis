#pragma once

#include <vtkSmartPointer.h>

#include <core/AbstractVisualizedData.h>


class vtkActor;
class vtkPropCollection;
class vtkScalarsToColors;

namespace reflectionzeug
{
    class PropertyGroup;
}

class DataObject;
class ScalarsForColorMapping;
enum class ContentType;


/**
Base class for rendered representations of loaded data objects.
A data object may be rendered in multiple views, each holding its own
RenderedData instance, referring to the data object.
*/ 
class CORE_API RenderedData : public AbstractVisualizedData
{
    Q_OBJECT

public:
    RenderedData(DataObject * dataObject);

    /** VTK view props visualizing the data set object and possibly additional attributes */
    vtkSmartPointer<vtkPropCollection> viewProps();

    virtual reflectionzeug::PropertyGroup * createConfigGroup() = 0;

    /** set scalars that will configure color mapping for this data */
    void setScalarsForColorMapping(ScalarsForColorMapping * scalars);
    /** Set gradient that will be applied to colored geometries.
      * ScalarsForColorMapping are responsible for gradient configuration. */
    void setColorMappingGradient(vtkScalarsToColors * gradient);

signals:
    void viewPropCollectionChanged();

protected:
    virtual vtkSmartPointer<vtkPropCollection> fetchViewProps() = 0;
    void invalidateViewProps();

    virtual void scalarsForColorMappingChangedEvent();
    virtual void colorMappingGradientChangedEvent();

protected:
    ScalarsForColorMapping * m_scalars;
    vtkSmartPointer<vtkScalarsToColors> m_gradient;

private:
    vtkSmartPointer<vtkPropCollection> m_viewProps;
    bool m_viewPropsInvalid;
};
