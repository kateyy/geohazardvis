#include "ColorBarRepresentation.h"

#include <cassert>

#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>

#include <core/color_mapping/ColorMapping.h>
#include <core/utility/ScalarBarActor.h>


ColorBarRepresentation::ColorBarRepresentation(ColorMapping & colorMapping)
    : m_colorMapping{ colorMapping }
    , m_isVisible{ false }
{
    connect(&colorMapping, &ColorMapping::currentScalarsChanged, this, &ColorBarRepresentation::updateForChangedScalars);
}

OrientedScalarBarActor & ColorBarRepresentation::actor()
{
    initialize();

    assert(m_actor);

    return *m_actor;
}

vtkScalarBarActor & ColorBarRepresentation::actorBase()
{
    return actor();
}

vtkScalarBarWidget & ColorBarRepresentation::widget()
{
    initialize();

    assert(m_widget);

    return *m_widget;
}

vtkScalarBarRepresentation & ColorBarRepresentation::scalarBarRepresentation()
{
    initialize();

    assert(m_scalarBarRepresentation);

    return *m_scalarBarRepresentation;
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

void ColorBarRepresentation::initialize()
{
    if (m_widget)
    {
        return;
    }

    m_actor = vtkSmartPointer<OrientedScalarBarActor>::New();
    m_actor->VisibilityOff();
    m_actor->SetLookupTable(m_colorMapping.scalarsToColors());
    m_actor->SetTitle(m_colorMapping.currentScalarsName().toUtf8().data());

    m_scalarBarRepresentation = vtkSmartPointer<vtkScalarBarRepresentation>::New();
    m_scalarBarRepresentation->SetScalarBarActor(m_actor);

    m_widget = vtkSmartPointer<vtkScalarBarWidget>::New();
    m_widget->SetScalarBarActor(m_actor);
    m_widget->SetRepresentation(m_scalarBarRepresentation);
    m_widget->EnabledOff();
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
    bool actualVisibilty = m_colorMapping.currentScalarsUseMappingLegend() && m_isVisible;

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
