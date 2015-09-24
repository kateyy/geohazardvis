#include "InteractorStyleImage.h"

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : Superclass()
{
}

InteractorStyleImage::~InteractorStyleImage() = default;

void InteractorStyleImage::OnLeftButtonDown()
{
    int eventPos[2];
    GetInteractor()->GetEventPosition(eventPos);
    FindPokedRenderer(eventPos[0], eventPos[1]);

    if (!GetCurrentRenderer())
        return;

    GrabFocus(EventCallbackCommand);

    StartPan();
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

void InteractorStyleImage::resetCamera()
{
    auto renderer = GetCurrentRenderer();
    if (!renderer)
    {
        return;
    }

    auto & camera = *renderer->GetActiveCamera();
    camera.SetViewUp(0, 1, 0);
    camera.SetFocalPoint(0, 0, 0);
    camera.SetPosition(0, 0, 1);
}

void InteractorStyleImage::moveCameraTo(AbstractVisualizedData & /*visualization*/, vtkIdType /*index*/, IndexType /*indexType*/, bool /*overTime*/)
{
    // TODO
}
