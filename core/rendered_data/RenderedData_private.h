/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
