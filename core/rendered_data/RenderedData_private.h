#pragma once

#include <core/AbstractVisualizedData_private.h>
#include <core/CoordinateSystems.h>
#include <core/rendered_data/RenderedData.h>

#include <vtkActor.h>
#include <vtkOutlineFilter.h>
#include <vtkPassThrough.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>


class CORE_API RenderedData_private : public AbstractVisualizedData_private
{
public:
    RenderedData_private(RenderedData & renderedData, ContentType contentType, DataObject & dataObject)
        : AbstractVisualizedData_private(renderedData, contentType, dataObject)
        , viewPropsInvalid{ true }
        , representation{ RenderedData::Representation::content }
        , outlineProperty{ vtkSmartPointer<vtkProperty>::New() }
        , transformedCoordinatesOutput{ vtkSmartPointer<vtkPassThrough>::New() }
    {
    }

    bool shouldShowContent() const;
    bool shouldShowOutline() const;

    vtkActor * outlineActor();

    vtkSmartPointer<vtkPropCollection> viewProps;
    bool viewPropsInvalid;

    RenderedData::Representation representation;

    vtkSmartPointer<vtkProperty> outlineProperty;
    vtkSmartPointer<vtkActor> m_outlineActor;

    CoordinateSystemSpecification defaultCoordinateSystem;
    vtkSmartPointer<vtkPassThrough> transformedCoordinatesOutput;

    RenderedData & qPtr() { return static_cast<RenderedData &>(q_ptr); }
    const RenderedData & qPtr() const { return static_cast<const RenderedData &>(q_ptr); }
};

bool RenderedData_private::shouldShowContent() const
{
    return qPtr().isVisible()
        && (representation == RenderedData::Representation::content
            || representation == RenderedData::Representation::both);
}

bool RenderedData_private::shouldShowOutline() const
{
    return qPtr().isVisible()
        && (representation == RenderedData::Representation::outline
            || representation == RenderedData::Representation::both);
}

vtkActor * RenderedData_private::outlineActor()
{
    if (m_outlineActor)
    {
        return m_outlineActor;
    }

    auto outline = vtkSmartPointer<vtkOutlineFilter>::New();
    outline->SetInputConnection(qPtr().transformedCoordinatesOutputPort());

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(outline->GetOutputPort());

    qPtr().setupInformation(*mapper->GetInformation(), qPtr());

    m_outlineActor = vtkSmartPointer<vtkActor>::New();
    m_outlineActor->SetMapper(mapper);
    m_outlineActor->SetProperty(outlineProperty);

    return m_outlineActor;
}
