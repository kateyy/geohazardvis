#pragma once

#include <core/AbstractVisualizedData.h>


class vtkActor;
class vtkPropCollection;
class vtkProperty;


/**
Base class for rendered representations of loaded data objects.
A data object may be rendered in multiple views, each holding its own
RenderedData instance, referring to the data object.
*/ 
class CORE_API RenderedData : public AbstractVisualizedData
{
    Q_OBJECT

public:
    RenderedData(ContentType contentType, DataObject & dataObject);
    ~RenderedData() override;

    enum class Representation { content, outline, both };

    Representation representation() const;
    void setRepresentation(Representation representation);

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    /** VTK view props visualizing the data set object and possibly additional attributes */
    vtkSmartPointer<vtkPropCollection> viewProps();

signals:
    void viewPropCollectionChanged();

protected:
    void visibilityChangedEvent(bool visible) override;
    virtual void representationChangedEvent(Representation representation);
    virtual vtkSmartPointer<vtkPropCollection> fetchViewProps() = 0;
    void invalidateViewProps();

private:
    bool shouldShowContent() const;
    bool shouldShowOutline() const;

    vtkActor * outlineActor();

private:
    vtkSmartPointer<vtkPropCollection> m_viewProps;
    bool m_viewPropsInvalid;

    Representation m_representation;

    vtkSmartPointer<vtkProperty> m_outlineProperty;
    vtkSmartPointer<vtkActor> m_outlineActor;

private:
    Q_DISABLE_COPY(RenderedData)
};
