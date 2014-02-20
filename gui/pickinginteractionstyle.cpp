#include "pickinginteractionstyle.h"

#include <vtkRenderWindowInteractor.h>

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <vtkPointPicker.h>

#include <gui/viewer.h>


vtkStandardNewMacro(PickingInteractionStyle);

PickingInteractionStyle::PickingInteractionStyle()
: m_pickingInfo()
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

    m_pickingInfo.sendPointInfo(picker);
}

void PickingInteractionStyle::setViewer(const Viewer & viewer)
{
    m_viewer = &viewer;
    QObject::connect(&m_pickingInfo, SIGNAL(infoSent(const QStringList&)), m_viewer, SLOT(ShowInfo(const QStringList&)));
}
