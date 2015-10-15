#pragma once

#include <string>
#include <unordered_map>

#include <vtkCommand.h>
#include <vtkInteractorStyle.h>
#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class GUI_API InteractorStyleSwitch : public vtkInteractorStyle
{
public:
    static InteractorStyleSwitch * New();
    vtkTypeMacro(InteractorStyleSwitch, vtkInteractorStyle);

    enum
    {
        StyleChangedEvent = vtkCommand::UserEvent + 1
    };

    void addStyle(const std::string & name, vtkInteractorStyle * interactorStyle);
    void setCurrentStyle(const std::string & name);
    const std::string & currentStyleName() const;
    vtkInteractorStyle * currentStyle();

    void SetInteractor(vtkRenderWindowInteractor * interactor) override;
    void SetAutoAdjustCameraClippingRange(int value) override;
    void SetDefaultRenderer(vtkRenderer * renderer) override;
    void SetCurrentRenderer(vtkRenderer * renderer) override;

protected:
    InteractorStyleSwitch();
    ~InteractorStyleSwitch() override;

    virtual void styleAddedEvent(vtkInteractorStyle * interactorStyle);
    virtual void currentStyleChangedEvent();
    const std::unordered_map<std::string, vtkSmartPointer<vtkInteractorStyle>> namedStyles() const;

private:
    std::unordered_map<std::string, vtkSmartPointer<vtkInteractorStyle>> m_namedStyles;
    std::string m_currentStyleName;
    vtkInteractorStyle * m_currentStyle;

public:
    InteractorStyleSwitch(const InteractorStyleSwitch&) = delete;
    void operator=(const InteractorStyleSwitch&) = delete;
};
