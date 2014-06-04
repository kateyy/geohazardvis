#pragma once

#include <memory>

#include <vtkSmartPointer.h>


class vtkProperty;
class vtkActor;

class DataObject;


/**
Base class for rendered representations of loaded data objects.
A data object may be rendered in multiple views, each holding its own
RenderedData instance, referring to the data object.
*/ 
class RenderedData
{
public:
    RenderedData(std::shared_ptr<const DataObject> dataObject);
    virtual ~RenderedData() = 0;

    std::shared_ptr<const DataObject> dataObject() const;

    vtkProperty * renderProperty();
    vtkActor * actor();

protected:
    virtual vtkProperty * createDefaultRenderProperty() const = 0;
    virtual vtkActor * createActor() const = 0;

private:
    const std::shared_ptr<const DataObject> m_dataObject;

    vtkSmartPointer<vtkProperty> m_renderProperty;
    vtkSmartPointer<vtkActor> m_actor;
};
