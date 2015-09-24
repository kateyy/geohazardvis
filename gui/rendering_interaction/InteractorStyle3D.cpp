#include "InteractorStyle3D.h"

#include <cassert>
#include <cmath>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

#include <core/utility/vtkcamerahelper.h>
#include <gui/rendering_interaction/CameraDolly.h>


vtkStandardNewMacro(InteractorStyle3D);

InteractorStyle3D::InteractorStyle3D()
    : Superclass()
    , m_cameraDolly(std::make_unique<CameraDolly>())
{
}

InteractorStyle3D::~InteractorStyle3D() = default;

void InteractorStyle3D::OnMouseMove()
{
    switch (this->State)
    {
    case VTKIS_ROTATE:
        if (!m_mouseMoved)
        {
            this->GrabFocus(this->EventCallbackCommand);
            m_mouseMoved = true;
        }
    }

    Superclass::OnMouseMove();
}

void InteractorStyle3D::OnLeftButtonDown()
{
    // Fix the superclass's method: only grab (exclusive) focus once the user moved the cursor
    // Otherwise, we would block left button down/release events from other observers

    this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                            this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == NULL)
    {
        return;
    }

    this->StartRotate();

    m_mouseMoved = false;
}

void InteractorStyle3D::OnLeftButtonUp()
{
    m_mouseMoved = false;

    Superclass::OnLeftButtonUp();
}

void InteractorStyle3D::OnMiddleButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyle3D::OnMiddleButtonUp()
{
    switch (State)
    {
    case VTKIS_DOLLY:
        EndDolly();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnMiddleButtonUp();
    }
}

void InteractorStyle3D::OnRightButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartPan();
}

void InteractorStyle3D::OnRightButtonUp()
{
    switch (State)
    {
    case VTKIS_PAN:
        EndPan();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnRightButtonUp();
    }
}

void InteractorStyle3D::OnMouseWheelForward()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    MouseWheelDolly(true);
}

void InteractorStyle3D::OnMouseWheelBackward()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    MouseWheelDolly(false);
}

void InteractorStyle3D::OnChar()
{
    // disable most magic keys

    if (this->Interactor->GetKeyCode() == 'l')
        Superclass::OnChar();
}

void InteractorStyle3D::resetCamera()
{
    auto renderer = GetCurrentRenderer();
    if (!renderer)
    {
        return;
    }

    auto & camera = *renderer->GetActiveCamera();
    camera.SetViewUp(0, 0, 1);
    TerrainCamera::setAzimuth(camera, 0);
    TerrainCamera::setVerticalElevation(camera, 45);
}

void InteractorStyle3D::moveCameraTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime)
{
    m_cameraDolly->setRenderer(GetCurrentRenderer());
    m_cameraDolly->moveTo(visualization, index, indexType, overTime);
}

void InteractorStyle3D::MouseWheelDolly(bool forward)
{
    if (!CurrentRenderer)
        return;

    GrabFocus(EventCallbackCommand);
    StartDolly();

    double factor = MotionFactor * 0.2 * MouseWheelMotionFactor;
    if (!forward)
        factor *= -1;
    factor = std::pow(1.1, factor);

    auto camera = CurrentRenderer->GetActiveCamera();
    if (camera->GetParallelProjection())
    {
        camera->SetParallelScale(camera->GetParallelScale() / factor);
    }
    else
    {
        camera->Dolly(factor);
        if (AutoAdjustCameraClippingRange)
        {
            CurrentRenderer->ResetCameraClippingRange();
        }
    }

    if (Interactor->GetLightFollowCamera())
    {
        CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }

    EndDolly();
    ReleaseFocus();

    Interactor->Render();
}
