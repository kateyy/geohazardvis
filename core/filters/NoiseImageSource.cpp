#include "NoiseImageSource.h"

#include <cassert>
#include <random>

#include <QFile>

#include <vtkDataObject.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <vtkFrameBufferObject2.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLError.h>
#include <vtkPixelBufferObject.h>
#include <vtkPixelTransfer.h>
#include <vtkRenderbuffer.h>
#include <vtkTextureObject.h>

#include <core/utility/vtkhelper.h>

#include "config.h"

#if VTK_RENDERING_BACKEND == 1
#include <vtkShader2.h>
#include <vtkShader2Collection.h>
#include <vtkShaderProgram2.h>
#endif


vtkStandardNewMacro(NoiseImageSource);


NoiseImageSource::NoiseImageSource()
    : Superclass()
    , ScalarsName{ nullptr }
    , NumberOfComponents{ 1 }
    , Seed{ 0 }
    , NoiseSource{ CpuUniformDistribution }
{
    Extent[0] = Extent[2] = Extent[4] = 0;
    Extent[1] = Extent[2] = 127;
    Extent[5] = 1;

    Origin[0] = Origin[1] = Origin[2] = 0.0;

    Spacing[0] = Spacing[1] = Spacing[2] = 1.0;

    ValueRange[0] = 0.0;
    ValueRange[1] = 1.0;

    SetNumberOfInputPorts(0);
    SetNumberOfOutputPorts(1);
}

NoiseImageSource::~NoiseImageSource() = default;


void NoiseImageSource::SetExtent(int extent[6])
{
    int description = vtkStructuredData::SetExtent(extent, this->Extent);

    if (description < 0)
        vtkErrorMacro(<< "Bad Extent, retaining previous values");

    if (description == VTK_UNCHANGED)
        return;

    Modified();
}

void NoiseImageSource::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
    int ext[6] = { x1, x2, y1, y2, z1, z2 };
    SetExtent(ext);
}

vtkIdType NoiseImageSource::GetNumberOfTuples() const
{
    int dims[3] = { Extent[1] - Extent[0] + 1, Extent[3] - Extent[2] + 1, Extent[5] - Extent[4] + 1 };

    return dims[0] * dims[1] * dims[2];
}

int NoiseImageSource::RequestInformation(vtkInformation * vtkNotUsed(request),
    vtkInformationVector ** vtkNotUsed(inputVector),
    vtkInformationVector * outputVector)
{
    vtkInformation * outInfo = outputVector->GetInformationObject(0);

    outInfo->Set(vtkDataObject::FIELD_NUMBER_OF_TUPLES(), static_cast<int>(GetNumberOfTuples()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), Extent, 6);

    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, NumberOfComponents);

    return 1;
}

int NoiseImageSource::RequestData(vtkInformation * vtkNotUsed(request),
    vtkInformationVector ** vtkNotUsed(inputVector),
    vtkInformationVector * outputVector)
{
    vtkInformation * outInfo = outputVector->GetInformationObject(0);
    vtkImageData * output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    assert(output);

    output->SetExtent(Extent);
    output->SetOrigin(Origin);
    output->SetSpacing(Spacing);
    output->AllocateScalars(VTK_FLOAT, NumberOfComponents);

    vtkDataArray * noiseData = output->GetPointData()->GetScalars();
    assert(noiseData);
    noiseData->SetName(ScalarsName);

    int result = 0;

    // try GPU noise if requested
    if (this->NoiseSource == GpuPerlinNoise)
    {
        result = ExecuteGpuPerlin(noiseData);

        if (result)
        {
            double range[2];
            noiseData->GetRange(range);
            if (range[0] >= 0 && range[1] <= 1)
                result = 1;
        }
    }

    if (result)
        return result;

    // make sure to have valid output data
    return ExecuteCPUUniformDist(noiseData);
}

int NoiseImageSource::ExecuteCPUUniformDist(vtkDataArray * data)
{
    std::mt19937 engine(Seed);
    std::uniform_real_distribution<double> rand(
        static_cast<double>(ValueRange[0]),
        static_cast<double>(ValueRange[1]));

    double * randTuple = new double[NumberOfComponents];

    vtkIdType numTuples = GetNumberOfTuples();
    for (vtkIdType i = 0; i < numTuples; ++i)
    {
        for (int comp = 0; comp < NumberOfComponents; ++comp)
            randTuple[comp] = rand(engine);

        data->SetTuple(i, randTuple);
    }

    delete[] randTuple;

    return 1;
}

#if VTK_RENDERING_BACKEND == 1
int NoiseImageSource::ExecuteGpuPerlin(vtkDataArray * data)
{
    if (Extent[4] != Extent[5])
        return 0;

    QFile shaderFile("data/Noise.frag");
    if (!shaderFile.open(QIODevice::ReadOnly))
    {
        std::cerr << "NoiseImageSource: could not open noise shader: " << shaderFile.fileName().toStdString() << std::endl;
            return 0;
    }

    QByteArray shaderSource = shaderFile.readAll();

    VTK_CREATE(vtkRenderWindow, window);
    window->OffScreenRenderingOn();

    vtkOpenGLRenderWindow * context = vtkOpenGLRenderWindow::SafeDownCast(window);
    if (!context)
    {
        std::cerr << "NoiseImageSource: OpenGL context required. " << shaderFile.fileName().toStdString() << std::endl;
        return 0;
    }

    context->MakeCurrent();

    unsigned int size[2] = {
        static_cast<unsigned int>(Extent[1] - Extent[0] + 1),
        static_cast<unsigned int>(Extent[3] - Extent[2] + 1)};

    glViewport(0, 0, size[0], size[1]);

    static const float normTexCoords[] = {
        +1.f, -1.f,
        +1.f, +1.f,
        -1.f, -1.f,
        -1.f, +1.f };

    struct Vertex
    {
        float pos[2];
        float tex[2];
    };
    Vertex vertexArrayData[4];

    for (int q = 0; q < 4; ++q)
    {
        int qq = 2 * q;

        Vertex & v = vertexArrayData[q];

        v.pos[0] = normTexCoords[qq];
        v.pos[1] = normTexCoords[qq + 1];
        v.tex[0] = normTexCoords[qq];
        v.tex[1] = normTexCoords[qq + 1];
    }

    VTK_CREATE(vtkShaderProgram2, prog);
    prog->SetContext(context);

    VTK_CREATE(vtkShader2, shader);
    shader->SetContext(context);
    shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    shader->SetSourceCode(shaderSource.data());

    prog->GetShaders()->AddItem(shader);

    prog->SetPrintErrors(true);
    prog->Build();

    if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
        std::cerr << "NoiseImageSource: Error in shader file: " << shaderFile.fileName().toStdString() << std::endl;
        return 0;
    }

    prog->Use();

    VTK_CREATE(vtkTextureObject, tex);
    tex->SetContext(context);
    tex->Create2D(size[0], size[1], 4, VTK_FLOAT, false);

    VTK_CREATE(vtkRenderbuffer, depth);
    depth->SetContext(context);
    depth->CreateDepthAttachment(size[0], size[1]);

    VTK_CREATE(vtkFrameBufferObject2, fbo);
    fbo->SetContext(context);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER, 0U, tex);
    fbo->AddRenDepthAttachment(vtkgl::DRAW_FRAMEBUFFER, depth->GetHandle());
    fbo->ActivateDrawBuffer(0U);
    if (!fbo->CheckFrameBufferStatus(vtkgl::FRAMEBUFFER))
    {
        std::cerr << "NoiseImageSource: Framebuffer incomplete." << std::endl;
        return 0;
    }

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < 4; ++i)
    {
        const Vertex & v = vertexArrayData[i];
        vtkgl::MultiTexCoord2f(vtkgl::TEXTURE0, v.tex[0], v.tex[1]);
        glVertex2f(v.pos[0], v.pos[1]);
    }
    glEnd();

    vtkOpenGLCheckErrorMacro("NoiseImageSource: Failed after draw");

    vtkPixelBufferObject * pbo = tex->Download();

    vtkPixelExtent ext(size[0], size[1]);
    vtkPixelTransfer::Blit(
        ext,
        ext,
        ext,
        ext,
        4,
        VTK_FLOAT,
        pbo->MapPackedBuffer(),
        NumberOfComponents,
        VTK_FLOAT,
        data->GetVoidPointer(0));

    pbo->UnmapPackedBuffer();
    pbo->Delete();

    vtkOpenGLCheckErrorMacro("NoiseImageSource: Failed after Blit to CPU");

    return 1;
}
#else
int NoiseImageSource::ExecuteGpuPerlin(vtkDataArray * /*data*/)
{
    return 0;
}
#endif
