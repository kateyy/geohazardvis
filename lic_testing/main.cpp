
#include <random>

#include <vtkExtractVOI.h>
#include <vtkImageData.h>
#include <vtkImageDataLIC2D.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkPointData.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vtkFloatArray.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>

#include <core/vtkhelper.h>
#include <core/Loader.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


int main()
{
    VectorGrid3DDataObject * grid = dynamic_cast<VectorGrid3DDataObject *>(Loader::readFile("D:/Documents/Studium/GFZ/data/dataSets_meta_2/DispVec_Volumetric_5slices.txt"));
    if (!grid)
        return 1;

    vtkImageData * image = vtkImageData::SafeDownCast(grid->dataSet());
    vtkDataArray * vectors = image->GetPointData()->GetVectors();
    double tuple[3];
    for (vtkIdType i = 0; i < vectors->GetNumberOfTuples(); ++i)
    {
        vectors->GetTuple(i, tuple);
        for (int c = 0; c < 3; ++c)
            tuple[c] *= 10000000;
        vectors->SetTuple(i, tuple);
    }
    double range[2];
    vectors->GetRange(range);

    int voi[6];
    image->GetExtent(voi);

    voi[5] = voi[4];

    VTK_CREATE(vtkExtractVOI, extractVoi);
    extractVoi->SetInputData(image);
    extractVoi->SetVOI(voi);
    extractVoi->Update();

    std::mt19937 mt(0);
    std::uniform_real_distribution<float> rand(0, 1);


    VTK_CREATE(vtkFloatArray, noiseData);
    noiseData->SetNumberOfComponents(2);
    noiseData->SetNumberOfTuples(128 * 128);
    for (vtkIdType i = 0; i < 128 * 128; ++i)
        noiseData->SetTuple2(i, rand(mt), rand(mt));

    VTK_CREATE(vtkImageData, noiseImage);
    noiseImage->SetExtent(0, 127, 0, 127, 0, 0);
    noiseImage->GetPointData()->SetScalars(noiseData);

    VTK_CREATE(vtkImageDataLIC2D, lic);
    lic->SetInputConnection(extractVoi->GetOutputPort());
    lic->SetInputData(1, noiseImage);
    lic->Update();

    auto a = lic->GetOutput();
    VTK_CREATE(vtkImageMapToColors, toColors);
    toColors->SetInputConnection(lic->GetOutputPort());
    toColors->SetOutputFormatToRGB();
    VTK_CREATE(vtkLookupTable, lut);
    lut->Build();
    toColors->SetLookupTable(lut);
    

    VTK_CREATE(vtkImageSliceMapper, mapper);
    mapper->SetInputConnection(toColors->GetOutputPort());
    mapper->SetOrientationToZ();
    mapper->SetSliceNumber(0);
    mapper->Update();
    
    VTK_CREATE(vtkImageSlice, slice);
    slice->SetMapper(mapper);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddViewProp(slice);
    renderer->SetBackground(0.9, 0.9, 0.9);
    renderer->ResetCamera();

    VTK_CREATE(vtkRenderWindow, window);
    window->AddRenderer(renderer);

    VTK_CREATE(vtkRenderWindowInteractor, interactor);
    interactor->SetRenderWindow(window);
    interactor->Initialize();

    interactor->Start();

    delete grid;

    return 0;
}