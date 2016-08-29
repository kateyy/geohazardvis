#include "CameraInteractorStyleSwitch.h"

#include <vtkObjectFactory.h>


vtkStandardNewMacro(CameraInteractorStyleSwitch);


CameraInteractorStyleSwitch::CameraInteractorStyleSwitch()
    : InteractorStyleSwitch()
    , m_currentCameraStyle{ nullptr }
{
}

CameraInteractorStyleSwitch::~CameraInteractorStyleSwitch() = default;

void CameraInteractorStyleSwitch::resetCameraToDefault(vtkCamera & camera)
{
    if (!m_currentCameraStyle)
    {
        return;
    }

    m_currentCameraStyle->resetCameraToDefault(camera);
}

void CameraInteractorStyleSwitch::moveCameraTo(const VisualizationSelection & selection, bool overTime)
{
    if (!m_currentCameraStyle)
    {
        return;
    }

    m_currentCameraStyle->moveCameraTo(selection, overTime);
}

void CameraInteractorStyleSwitch::currentStyleChangedEvent()
{
    m_currentCameraStyle = dynamic_cast<ICameraInteractionStyle *>(currentStyle());
}
