#pragma once

#include <QObject>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <core/core_api.h>


class ColorMapping;
class OrientedScalarBarActor;
class vtkScalarBarActor;
class vtkScalarBarRepresentation;
class vtkScalarBarWidget;
class vtkRenderer;
class vtkRenderWindowInteractor;


/** Manage the representation of a color bar.

This class ensures that the color bar is only visible, if the related object is visible
and the current color mapping actually makes use of the color bar.
*/
class CORE_API ColorBarRepresentation : public QObject
{
    Q_OBJECT

public:
    explicit ColorBarRepresentation(ColorMapping & colorMapping);

    OrientedScalarBarActor & actor();
    /** Convenience function retuning actor() as a base class reference */
    vtkScalarBarActor & actorBase();
    vtkScalarBarWidget & widget();
    vtkScalarBarRepresentation & scalarBarRepresentation();

    /** get/set visibility of the color mapping legend.
      * It is always hidden if the current scalars don't use the color mapping. */
    bool isVisible() const;
    void setVisible(bool visible);

    /** This is required in order to enable correct visualization and interaction */
    void setContext(vtkRenderWindowInteractor * interactor, vtkRenderer * renderer);

signals:
    void colorBarVisibilityChanged(bool visible);

private:
    void initialize();

    void updateForChangedScalars();
    void updateVisibility();
    void updateForChangedContext();

private:
    ColorMapping & m_colorMapping;

    vtkSmartPointer<OrientedScalarBarActor> m_actor;
    vtkSmartPointer<vtkScalarBarRepresentation> m_scalarBarRepresentation;
    vtkSmartPointer<vtkScalarBarWidget> m_widget;
    bool m_isVisible;

    vtkWeakPointer<vtkRenderWindowInteractor> m_interactor;
    vtkWeakPointer<vtkRenderer> m_renderer;
};
