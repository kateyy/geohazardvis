#pragma once

#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>


class CORE_API GridAxes3DActor : public vtkGridAxes3DActor
{
public:
    vtkTypeMacro(GridAxes3DActor, vtkGridAxes3DActor);

    static GridAxes3DActor * New();

    vtkGetMacro(LabelsVisible, bool);
    void SetLabelsVisible(bool visible);
    vtkBooleanMacro(LabelsVisible, bool);

    void SetLabelMask(unsigned int mask) override;

protected:
    GridAxes3DActor();
    ~GridAxes3DActor() override;

private:
    bool LabelsVisible;
    unsigned int RequestedLabelMask;

private:
    GridAxes3DActor(const GridAxes3DActor &) = delete;
    void operator=(const GridAxes3DActor &) = delete;
};
