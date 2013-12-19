#include "pickinginteractionstyle.h"

#include <vtkRenderWindowInteractor.h>

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkPointPicker.h>

#include <gui/viewer.h>


vtkStandardNewMacro(PickingInteractionStyle);

PickingInteractionStyle::PickingInteractionStyle()
: m_pickingInfo(PickingInfo())
{
}

void PickingInteractionStyle::OnLeftButtonDown()
{
    int* clickPos = this->GetInteractor()->GetEventPosition();

    //{   // fast picking with help of the render buffer
    //    vtkSmartPointer<vtkPropPicker>  picker =
    //        vtkSmartPointer<vtkPropPicker>::New();

    //    picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());
    //    double* pos = picker->GetPickPosition();
    //    std::cout << "vtkPropPicker world pos:" << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
    //    std::cout << "\tPicked actor: " << picker->GetActor() << std::endl;
    //}

    //{   // picking in the input geometry
    //    vtkSmartPointer<vtkCellPicker> picker =
    //        vtkSmartPointer<vtkCellPicker>::New();

    //    picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());
    //    double* pos = picker->GetPickPosition();
    //    std::cout << "vtkCellPicker world pos:" << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
    //    std::cout << "\tPicked actor: " << picker->GetActor() << std::endl;
    //}
    {   // picking in the input geometry
        vtkSmartPointer<vtkPointPicker> picker = vtkSmartPointer<vtkPointPicker>::New();

        picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

        m_pickingInfo.sendPointInfo(picker);
    }

    // Forward events
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void PickingInteractionStyle::setViewer(const Viewer & viewer)
{
    m_viewer = &viewer;
    QObject::connect(&m_pickingInfo, SIGNAL(infoSent(QString)), m_viewer, SLOT(ShowInfo(QString)));
}
