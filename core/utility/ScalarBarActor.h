#pragma once

#include <core/ThirdParty/ParaView/vtkPVScalarBarActor.h>


class CORE_API OrientedScalarBarActor : public vtkPVScalarBarActor
{
public:
    vtkTypeMacro(OrientedScalarBarActor, vtkPVScalarBarActor);
    void PrintSelf(ostream &os, vtkIndent indent) override;
    static OrientedScalarBarActor * New();

    vtkGetMacro(TitleAlignedWithColorBar, bool);
    vtkSetMacro(TitleAlignedWithColorBar, bool)

protected:
    OrientedScalarBarActor();
    ~OrientedScalarBarActor();

    void LayoutTitle() override;
    void ConfigureTitle() override;

protected:
    bool TitleAlignedWithColorBar;

private:
    OrientedScalarBarActor(const OrientedScalarBarActor &) = delete;
    void operator=(const OrientedScalarBarActor &) = delete;
};
