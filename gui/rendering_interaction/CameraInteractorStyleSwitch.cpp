#include "CameraInteractorStyleSwitch.h"

#include <vtkObjectFactory.h>


vtkStandardNewMacro(CameraInteractorStyleSwitch);


CameraInteractorStyleSwitch::CameraInteractorStyleSwitch()
    : InteractorStyleSwitch()
    , m_currentCameraStyle(nullptr)
{
}

CameraInteractorStyleSwitch::~CameraInteractorStyleSwitch() = default;

void CameraInteractorStyleSwitch::resetCamera()
{
    if (!m_currentCameraStyle)
        return;

    m_currentCameraStyle->resetCamera();
}

void CameraInteractorStyleSwitch::moveCameraTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime)
{
    if (!m_currentCameraStyle)
        return;

    m_currentCameraStyle->moveCameraTo(visualization, index, indexType, overTime);
}

void CameraInteractorStyleSwitch::currentStyleChangedEvent()
{
    m_currentCameraStyle = dynamic_cast<ICameraInteractionStyle *>(currentStyle());
}