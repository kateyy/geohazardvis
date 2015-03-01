#include <core/filters/StackedImageDataLIC3D.h>

#include <cassert>

#include <vtkExtractVOI.h>
#include <vtkImageAppend.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLExtensionManager.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkPointData.h>

#include <core/vtkhelper.h>
#include <core/filters/NoiseImageSource.h>
#include <core/filters/vtkImageDataLIC2D.h>


vtkStandardNewMacro(StackedImageDataLIC3D);


StackedImageDataLIC3D::StackedImageDataLIC3D()
    : Superclass()
    , m_isInitialized(false)
    , m_noiseImage(nullptr)
    , m_glContext(nullptr)
{
}

StackedImageDataLIC3D::~StackedImageDataLIC3D()
{
    if (m_noiseImage)
        m_noiseImage->Delete();

    if (m_glContext)
        m_glContext->Delete();
}

void StackedImageDataLIC3D::SimpleExecute(vtkImageData * input, vtkImageData * output)
{
    vtkDataArray * vectors = input->GetPointData()->GetVectors();
    if (!vectors)
    {
        vtkGenericWarningMacro("StackeImageDataLIC3D::SimpleExecute: missing input vectors");
        return;
    }

    initialize();

    VTK_CREATE(vtkImageAppend, appendTo3D);
    appendTo3D->SetAppendAxis(2);

    int inputExtent[6];
    input->GetExtent(inputExtent);

    for (int position = inputExtent[4]; position < inputExtent[5]; ++position)
    {
        int sliceExtent[6];
        input->GetExtent(sliceExtent);
        sliceExtent[4] = sliceExtent[5] = position;

        VTK_CREATE(vtkExtractVOI, slice);
        slice->SetVOI(sliceExtent);
        slice->SetInputData(input);

        VTK_CREATE(vtkImageDataLIC2D, lic2D);
        lic2D->SetInputConnection(0, slice->GetOutputPort());
        lic2D->SetInputConnection(1, m_noiseImage->GetOutputPort());
        lic2D->SetSteps(50);
        lic2D->GlobalWarningDisplayOff();
        lic2D->SetContext(m_glContext);

        appendTo3D->AddInputConnection(lic2D->GetOutputPort());
    }

    appendTo3D->Update();
    output->DeepCopy(appendTo3D->GetOutput());
}

void StackedImageDataLIC3D::initialize()
{
    if (m_isInitialized)
        return;


    m_glContext = vtkRenderWindow::New();
    vtkOpenGLRenderWindow * openGLContext = vtkOpenGLRenderWindow::SafeDownCast(m_glContext);
    assert(openGLContext);
    openGLContext->GetExtensionManager()->IgnoreDriverBugsOn(); // required for Intel HD
    openGLContext->OffScreenRenderingOn();

    m_noiseImage = NoiseImageSource::New();
    m_noiseImage->SetExtent(0, 1270, 0, 1270, 0, 0);
    m_noiseImage->SetValueRange(0, 1);
    m_noiseImage->SetNumberOfComponents(2);


    m_isInitialized = true;
}
