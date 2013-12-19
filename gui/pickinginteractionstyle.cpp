#include "pickinginteractionstyle.h"

#include <iostream>

#include "pickinginteractionstyle.h"

#include <vtkRenderWindowInteractor.h>

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkPointPicker.h>

#include <vtkActor.h>
#include <vtkActor2D.h>

#include <vtkMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkAlgorithmOutput.h>


vtkStandardNewMacro(PickingInteractionStyle);

void PickingInteractionStyle::OnLeftButtonDown()
{
    int* clickPos = this->GetInteractor()->GetEventPosition();

    {   // fast picking with help of the render buffer
        vtkSmartPointer<vtkPropPicker>  picker =
            vtkSmartPointer<vtkPropPicker>::New();

        picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());
        double* pos = picker->GetPickPosition();
        std::cout << "vtkPropPicker world pos:" << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
        std::cout << "\tPicked actor: " << picker->GetActor() << std::endl;
    }

    {   // picking in the input geometry
        vtkSmartPointer<vtkCellPicker> picker =
            vtkSmartPointer<vtkCellPicker>::New();

        picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());
        double* pos = picker->GetPickPosition();
        std::cout << "vtkCellPicker world pos:" << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
        std::cout << "\tPicked actor: " << picker->GetActor() << std::endl;
    }
    {   // picking in the input geometry
        vtkSmartPointer<vtkPointPicker> picker =
            vtkSmartPointer<vtkPointPicker>::New();

        picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());
        double* pos = picker->GetPickPosition();
        std::cout << "vtkPointPicker world pos:" << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
        std::cout << "\tPicked actor: " << picker->GetActor() << std::endl;
    }


    // Forward events
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}
