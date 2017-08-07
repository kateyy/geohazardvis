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

#include "ColorBarRepresentation.h"

#include <algorithm>
#include <cassert>

#include <vtkCommand.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarWidget.h>
#include <vtkTextProperty.h>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/ThirdParty/ParaView/vtkContext2DScalarBarActor.h>
#include <core/ThirdParty/ParaView/vtkPVScalarBarRepresentation.h>
#include <core/utility/font.h>
#include <core/utility/qthelper.h>


ColorBarRepresentation::ColorBarRepresentation(ColorMapping & colorMapping)
    : QObject()
    , m_colorMapping{ colorMapping }
    , m_isVisible{ false }
    , m_position{ Position::posRight }
    , m_inAdjustPosition{ false }
{
    connect(&colorMapping, &ColorMapping::currentScalarsChanged, this, &ColorBarRepresentation::updateForChangedScalars);

    auto connectVisibilities = [this] ()
    {
        disconnectAll(m_visualizationsVisibilitesConnections);

        for (auto && vis : m_colorMapping.visualizedData())
        {
            m_visualizationsVisibilitesConnections.emplace_back(
                connect(vis, &AbstractVisualizedData::visibilityChanged,
                    this, &ColorBarRepresentation::updateVisibility));
        }
    };

    connectVisibilities();
}

ColorBarRepresentation::~ColorBarRepresentation() = default;

vtkContext2DScalarBarActor & ColorBarRepresentation::actorContext2D()
{
    initialize();
    assert(m_actor);
    return *m_actor;
}

vtkScalarBarActor & ColorBarRepresentation::actor()
{
    return actorContext2D();
}

vtkScalarBarWidget & ColorBarRepresentation::widget()
{
    initialize();

    assert(m_widget);

    return *m_widget;
}

vtkPVScalarBarRepresentation & ColorBarRepresentation::scalarBarRepresentationPV()
{
    initialize();
    assert(m_scalarBarRepresentation);
    return *m_scalarBarRepresentation;
}

vtkScalarBarRepresentation & ColorBarRepresentation::scalarBarRepresentation()
{
    return scalarBarRepresentationPV();
}

bool ColorBarRepresentation::isVisible() const
{
    return m_isVisible;
}

void ColorBarRepresentation::setVisible(bool visible)
{
    if (visible == m_isVisible)
    {
        return;
    }

    m_isVisible = visible;

    updateVisibility();
}

void ColorBarRepresentation::setContext(vtkRenderWindowInteractor * interactor, vtkRenderer * renderer)
{
    m_interactor = interactor;
    m_renderer = renderer;

    updateForChangedContext();
}

void ColorBarRepresentation::setPosition(const Position position)
{
    if (m_inAdjustPosition)
    {
        return;
    }

    const bool changed = position != m_position;

    m_position = position;

    m_inAdjustPosition = true;

    applyPosition();

    if (changed)
    {
        emit positionChanged(m_position);
    }

    m_inAdjustPosition = false;
}

void ColorBarRepresentation::applyPosition()
{
    if (!m_scalarBarRepresentation)
    {
        return;
    }

    auto & scalarBarRepr = *m_scalarBarRepresentation;

    switch (m_position)
    {
    case ColorBarRepresentation::Position::posLeft:
        scalarBarRepr.SetOrientation(1);
        scalarBarRepr.SetPosition(0.02, 0.05);
        scalarBarRepr.SetPosition2(0.1, 0.5);
        break;
    case ColorBarRepresentation::Position::posRight:
        scalarBarRepr.SetOrientation(1);
        scalarBarRepr.SetPosition(0.9, 0.05);
        scalarBarRepr.SetPosition2(0.1, 0.5);
        break;
    case ColorBarRepresentation::Position::posTop:
        scalarBarRepr.SetOrientation(0);
        scalarBarRepr.SetPosition(0.3, 0.85);
        scalarBarRepr.SetPosition2(0.6, 0.1);
        break;
    case ColorBarRepresentation::Position::posBottom:
        scalarBarRepr.SetOrientation(0);
        scalarBarRepr.SetPosition(0.3, 0.01);
        scalarBarRepr.SetPosition2(0.6, 0.1);
        break;
    case ColorBarRepresentation::Position::posUserDefined:
        break;
    }
}

auto ColorBarRepresentation::position() const -> Position
{
    return m_position;
}

void ColorBarRepresentation::positionChangedEvent()
{
    if (m_inAdjustPosition)
    {
        return;
    }
    setPosition(Position::posUserDefined);
}

void ColorBarRepresentation::initialize()
{
    if (m_widget)
    {
        return;
    }

    m_actor = vtkSmartPointer<vtkContext2DScalarBarActor>::New();
    m_actor->VisibilityOff();
    m_actor->SetLookupTable(m_colorMapping.scalarsToColors());
    m_actor->SetTitle(m_colorMapping.currentScalarsName().toUtf8().data());
    m_actor->SetRangeLabelFormat("%.3g");
    m_actor->SetLabelFormat(m_actor->GetRangeLabelFormat());
    m_actor->AutomaticLabelFormatOff();
    FontHelper::configureTextProperty(*m_actor->GetTitleTextProperty());
    FontHelper::configureTextProperty(*m_actor->GetLabelTextProperty());
    FontHelper::configureTextProperty(*m_actor->GetAnnotationTextProperty());

    m_actor->GetTitleTextProperty()->SetFontSize(12);
    m_actor->GetTitleTextProperty()->ShadowOff();
    m_actor->GetTitleTextProperty()->SetColor(0, 0, 0);
    m_actor->GetTitleTextProperty()->BoldOff();
    m_actor->GetTitleTextProperty()->ItalicOff();

    m_actor->GetLabelTextProperty()->SetFontSize(10);
    m_actor->GetLabelTextProperty()->ShadowOff();
    m_actor->GetLabelTextProperty()->SetColor(0, 0, 0);
    m_actor->GetLabelTextProperty()->BoldOff();
    m_actor->GetLabelTextProperty()->ItalicOff();

    m_actor->SetNumberOfLabels(3);

    m_scalarBarRepresentation = vtkSmartPointer<vtkPVScalarBarRepresentation>::New();
    m_scalarBarRepresentation->SetScalarBarActor(m_actor);
    m_scalarBarRepresentation->SetShowBorderToActive();
    m_scalarBarRepresentation->GetBorderProperty()->SetColor(0, 0, 0);

    m_widget = vtkSmartPointer<vtkScalarBarWidget>::New();
    m_widget->SetScalarBarActor(m_actor);
    m_widget->SetRepresentation(m_scalarBarRepresentation);
    m_widget->SetInteractor(m_interactor);
    m_widget->KeyPressActivationOff();
    m_widget->EnabledOff();

    applyPosition();

    auto addObserver = [this] (vtkObject * subject, void(ColorBarRepresentation::* callback)())
    {
        subject->AddObserver(vtkCommand::ModifiedEvent, this, callback);
    };

    addObserver(m_actor->GetPositionCoordinate(), &ColorBarRepresentation::positionChangedEvent);
    addObserver(m_actor->GetPosition2Coordinate(), &ColorBarRepresentation::positionChangedEvent);

    const auto pos = m_position;
    m_position = posUserDefined;
    setPosition(pos);
}

void ColorBarRepresentation::updateForChangedScalars()
{
    if (m_actor)
    {
        m_actor->SetTitle(m_colorMapping.currentScalarsName().toUtf8().data());
    }

    updateVisibility();
}

void ColorBarRepresentation::updateVisibility()
{
    initialize();

    auto && viss = m_colorMapping.visualizedData();

    const bool actualVisibilty = m_colorMapping.currentScalarsUseMappingLegend() && m_isVisible
        && std::any_of(viss.begin(), viss.end(), [] (AbstractVisualizedData * vis) -> bool {
        return vis->isVisible(); });

    if (!actualVisibilty && !m_actor)
    {
        return;
    }

    const bool oldValue = m_actor->GetVisibility() != 0;

    if (oldValue == actualVisibilty)
    {
        return;
    }

    m_actor->SetVisibility(actualVisibilty);

    if (actualVisibilty)
    {
        // vtkAbstractWidget clears the current renderer when disabling and uses FindPokedRenderer while enabling
        // so ensure to always use the correct (not necessarily lastly clicked) renderer
        m_widget->SetCurrentRenderer(m_renderer);
    }

    emit colorBarVisibilityChanged(actualVisibilty);

    m_widget->SetEnabled(actualVisibilty);
}

void ColorBarRepresentation::updateForChangedContext()
{
    if (!m_widget)
    {
        return;
    }

    m_widget->SetCurrentRenderer(m_renderer);
}
