#include "NoiseImageSource.h"

#include <cassert>
#include <random>

#include <vtkDataObject.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkStandardNewMacro(NoiseImageSource);


NoiseImageSource::NoiseImageSource()
    : Superclass()
    , ScalarsName{ nullptr }
    , NumberOfComponents{ 1 }
    , Seed{ 0 }
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

    outInfo->Set(vtkDataObject::FIELD_NUMBER_OF_TUPLES(), GetNumberOfTuples());
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

        noiseData->SetTuple(i, randTuple);
    }

    delete[] randTuple;

    return 1;
}
