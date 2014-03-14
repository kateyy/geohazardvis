#include "pickinginteractionstyle.h"

#include <vtkRenderWindowInteractor.h>

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <vtkPointPicker.h>


vtkStandardNewMacro(PickingInteractionStyle);

PickingInteractionStyle::PickingInteractionStyle()
: vtkInteractorStyleTrackballCamera()
, pickingInfo()
{
}

void PickingInteractionStyle::OnMouseMove()
{
    pick();
    vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void PickingInteractionStyle::OnLeftButtonDown()
{
    pick();
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void PickingInteractionStyle::pick()
{
    int* clickPos = this->GetInteractor()->GetEventPosition();

    // picking in the input geometry
    vtkSmartPointer<vtkPointPicker> picker = vtkSmartPointer<vtkPointPicker>::New();

    picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

    pickingInfo.sendPointInfo(picker);
}
