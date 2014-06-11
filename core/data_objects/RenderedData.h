#pragma once

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class QImage;
class vtkProperty;
class vtkActor;

class DataObject;
class ScalarsForColorMapping;


/**
Base class for rendered representations of loaded data objects.
A data object may be rendered in multiple views, each holding its own
RenderedData instance, referring to the data object.
*/ 
class CORE_API RenderedData
{
public:
    RenderedData(DataObject * dataObject);
    virtual ~RenderedData() = 0;

    DataObject * dataObject();
    const DataObject * dataObject() const;

    vtkProperty * renderProperty();
    vtkActor * actor();

    void applyScalarsForColorMapping(ScalarsForColorMapping * scalars);
    void applyColorGradient(const QImage * gradient);

protected:
    virtual vtkProperty * createDefaultRenderProperty() const = 0;
    virtual vtkActor * createActor() const = 0;

    virtual void updateScalarToColorMapping() = 0;

protected:
    const ScalarsForColorMapping * m_scalars;
    const QImage * m_gradient;

private:
    DataObject * m_dataObject;

    vtkSmartPointer<vtkProperty> m_renderProperty;
    vtkSmartPointer<vtkActor> m_actor;
};
