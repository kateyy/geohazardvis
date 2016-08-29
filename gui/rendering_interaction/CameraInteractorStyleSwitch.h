#pragma once

#include <gui/rendering_interaction/InteractorStyleSwitch.h>
#include <gui/rendering_interaction/ICameraInteractionStyle.h>


class GUI_API CameraInteractorStyleSwitch : public InteractorStyleSwitch, virtual public ICameraInteractionStyle
{
public:
    static CameraInteractorStyleSwitch * New();
    vtkTypeMacro(CameraInteractorStyleSwitch, InteractorStyleSwitch);

    void resetCameraToDefault(vtkCamera & camera) override;
    void moveCameraTo(const VisualizationSelection & selection, bool overTime = true) override;
    
protected:
    CameraInteractorStyleSwitch();
    ~CameraInteractorStyleSwitch() override;

    void currentStyleChangedEvent() override;

private:
    ICameraInteractionStyle * m_currentCameraStyle;

private:
    CameraInteractorStyleSwitch(const CameraInteractorStyleSwitch &) = delete;
    void operator=(const CameraInteractorStyleSwitch &) = delete;
};
