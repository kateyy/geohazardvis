#pragma once

#include <core/AbstractVisualizedData.h>


class vtkPropCollection;


/**
Base class for rendered representations of loaded data objects.
A data object may be rendered in multiple views, each holding its own
RenderedData instance, referring to the data object.
*/ 
class CORE_API RenderedData : public AbstractVisualizedData
{
    Q_OBJECT

public:
    RenderedData(ContentType contentType, DataObject * dataObject);

    /** VTK view props visualizing the data set object and possibly additional attributes */
    vtkSmartPointer<vtkPropCollection> viewProps();

signals:
    void viewPropCollectionChanged();

protected:
    virtual vtkSmartPointer<vtkPropCollection> fetchViewProps() = 0;
    void invalidateViewProps();

private:
    vtkSmartPointer<vtkPropCollection> m_viewProps;
    bool m_viewPropsInvalid;
};
