#include "InteractorStyleImage.h"

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : Superclass()
    , m_mouseMoved(false)
    , m_mouseButtonDown(false)
{
}

InteractorStyleImage::~InteractorStyleImage() = default;

void InteractorStyleImage::OnMouseMove()
{
    if (m_mouseButtonDown && !m_mouseMoved)
    {
        GrabFocus(EventCallbackCommand);

        m_mouseMoved = true;
    }
    
    Superclass::OnMouseMove();
}

void InteractorStyleImage::OnLeftButtonDown()
{
    int eventPos[2];
    GetInteractor()->GetEventPosition(eventPos);
    FindPokedRenderer(eventPos[0], eventPos[1]);

    if (!GetCurrentRenderer())
        return;

    m_mouseMoved = false;
    m_mouseButtonDown = true;

    StartPan();
}

void InteractorStyleImage::OnLeftButtonUp()
{
    m_mouseMoved = false;
    m_mouseButtonDown = false;

    Superclass::OnLeftButtonUp();
}

void InteractorStyleImage::OnMiddleButtonDown()
{
    int eventPos[2];
    GetInteractor()->GetEventPosition(eventPos);
    FindPokedRenderer(eventPos[0], eventPos[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyleImage::OnMiddleButtonUp()
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

void InteractorStyleImage::OnRightButtonDown()
{
    OnLeftButtonDown();
}

void InteractorStyleImage::OnRightButtonUp()
{
    OnLeftButtonUp();
}

void InteractorStyleImage::OnChar()
{
    // disable magic keys for now
}

void InteractorStyleImage::resetCameraToDefault(vtkCamera & camera)
{
    camera.SetViewUp(0, 1, 0);
    camera.SetFocalPoint(0, 0, 0);
    camera.SetPosition(0, 0, 1);
    camera.ParallelProjectionOn();
}

void InteractorStyleImage::moveCameraTo(AbstractVisualizedData & /*visualization*/, vtkIdType /*index*/, IndexType /*indexType*/, bool /*overTime*/)
{
    // TODO
}
