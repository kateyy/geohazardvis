
#include <vtkSmartPointer.h>
#include <vtkStructuredPoints.h>
#include <vtkExtractVOI.h>
#include <vtkImageToPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkImageMapper.h>
#include <vtkActor.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#define VTK_CREATE(CLASS, NAME) vtkSmartPointer<CLASS> NAME = vtkSmartPointer<CLASS>::New();

int main()
{
    VTK_CREATE(vtkStructuredPoints, grid);
    grid->SetExtent(0, 10, 0, 10, 0, 0);

    VTK_CREATE(vtkExtractVOI, extract);
    extract->SetInputData(grid);
    
    VTK_CREATE(vtkImageToPolyDataFilter, polyData);
    polyData->SetInputConnection(extract->GetOutputPort());
    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(polyData->GetOutputPort());
    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(mapper);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor);

    VTK_CREATE(vtkRenderWindow, rw);
    rw->AddRenderer(renderer);

    VTK_CREATE(vtkRenderWindowInteractor, interactor);
    rw->SetInteractor(interactor);
    interactor->Initialize();
    interactor->Render();

    extract->SetSampleRate(5, 1, 1);

    interactor->Start();

    return 0;
}
