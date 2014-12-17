#pragma once

#include <vtkImageAlgorithm.h>

#include <core/core_api.h>


class CORE_API NoiseImageSource : public vtkImageAlgorithm
{
public:
    static NoiseImageSource * New();
    vtkTypeMacro(NoiseImageSource, vtkImageAlgorithm);


    vtkSetStringMacro(ScalarsName);
    vtkGetStringMacro(ScalarsName);

    vtkSetClampMacro(NumberOfComponents, int, 1, VTK_INT_MAX);
    vtkGetMacro(NumberOfComponents, int);

    vtkSetVector2Macro(ValueRange, double);
    vtkGetVector2Macro(ValueRange, double);

    vtkSetMacro(Seed, unsigned long);
    vtkGetMacro(Seed, unsigned long);

    virtual void SetExtent(int extent[6]);
    virtual void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);

    vtkSetVector3Macro(Origin, double);
    vtkGetVector3Macro(Origin, double);

    vtkSetVector3Macro(Spacing, double);
    vtkGetVector3Macro(Spacing, double);

    vtkGetVector6Macro(Extent, int);

protected:
    NoiseImageSource();
    ~NoiseImageSource() override;

    vtkIdType GetNumberOfTuples() const;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    NoiseImageSource(const NoiseImageSource&);  // Not implemented.
    void operator=(const NoiseImageSource&);  // Not implemented.

    char * ScalarsName;
    int NumberOfComponents;
    unsigned long Seed;

    int Extent[6];
    double Origin[3];
    double Spacing[3];
    double ValueRange[2];
};
