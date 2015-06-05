#pragma once

#include <core/ThirdParty/vtkPVScalarBarActor.h>


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
    void LayoutTitle() override;
    void ConfigureTitle() override;

protected:
    bool TitleAlignedWithColorBar;

};
