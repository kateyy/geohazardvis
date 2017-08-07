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

#include <vector>

#include <QObject>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <core/core_api.h>


class ColorMapping;
class vtkContext2DScalarBarActor;
class vtkPVScalarBarRepresentation;
class vtkScalarBarActor;
class vtkScalarBarRepresentation;
class vtkScalarBarWidget;
class vtkRenderer;
class vtkRenderWindowInteractor;


/**
 * Manage the representation of a color bar.
 *
 * This class ensures that the color bar is only visible, if the related object is visible and the
 * current color mapping actually makes use of the color bar.
 */
class CORE_API ColorBarRepresentation : public QObject
{
    Q_OBJECT

public:
    explicit ColorBarRepresentation(ColorMapping & colorMapping);
    ~ColorBarRepresentation() override;

    /** @return Scalar bar actor as its concrete class. */
    vtkContext2DScalarBarActor & actorContext2D();
    /** Convenience method returning general base class of actorContext2D() */
    vtkScalarBarActor & actor();
    /** @return Widget allowing users to drag and resize the color bar. */
    vtkScalarBarWidget & widget();
    /** @return Visual representation of the scalar bar. */
    vtkPVScalarBarRepresentation & scalarBarRepresentationPV();
    /** Convenience method returning the general base class of scalarBarRepresentationPV() */
    vtkScalarBarRepresentation & scalarBarRepresentation();

    /**
     * Get/set visibility of the color mapping legend.
     * It is always hidden if the current scalars don't use the color mapping.
     */
    bool isVisible() const;
    void setVisible(bool visible);

    /** This is required in order to enable correct visualization and interaction */
    void setContext(vtkRenderWindowInteractor * interactor, vtkRenderer * renderer);

    enum Position : int
    {
        posLeft, posRight, posTop, posBottom, posUserDefined
    };
    /** Move the color bar to a predefined position near one of the borders of the renderer. */
    void setPosition(Position position);
    Position position() const;

signals:
    void colorBarVisibilityChanged(bool visible);
    void positionChanged(Position position);

protected:
    virtual void positionChangedEvent();
    virtual void sizeChangedEvent();

private:
    void initialize();
    void applyPosition();

    void updateForChangedScalars();
    void updateVisibility();
    void updateForChangedContext();

private:
    ColorMapping & m_colorMapping;
    std::vector<QMetaObject::Connection> m_visualizationsVisibilitesConnections;

    vtkSmartPointer<vtkContext2DScalarBarActor> m_actor;
    vtkSmartPointer<vtkPVScalarBarRepresentation> m_scalarBarRepresentation;
    vtkSmartPointer<vtkScalarBarWidget> m_widget;
    bool m_isVisible;
    Position m_position;
    bool m_inAdjustPosition;

    vtkWeakPointer<vtkRenderWindowInteractor> m_interactor;
    vtkWeakPointer<vtkRenderer> m_renderer;
};

Q_DECLARE_METATYPE(ColorBarRepresentation::Position)
