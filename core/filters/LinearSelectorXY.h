#pragma once

#include <vtkSelectionAlgorithm.h>
#include <vtkVector.h>

#include <core/core_api.h>


class vtkPolyData;


class CORE_API LinearSelectorXY : public vtkSelectionAlgorithm
{
public:
    vtkTypeMacro(LinearSelectorXY, vtkSelectionAlgorithm);

    static LinearSelectorXY * New();

    void SetCellCentersConnection(vtkAlgorithmOutput * algOutput)
    {
        this->SetInputConnection(1, algOutput);
    }

    vtkPolyData * GetExtractedPoints();

    vtkGetMacro(StartPoint, vtkVector2d);
    vtkSetMacro(StartPoint, vtkVector2d);

    vtkGetMacro(EndPoint, vtkVector2d);
    vtkSetMacro(EndPoint, vtkVector2d);

    vtkGetMacro(OutputPositionOnLine, bool);
    vtkSetMacro(OutputPositionOnLine, bool);
    vtkBooleanMacro(OutputPositionOnLine, bool);

    vtkGetMacro(ComputeDistanceToLine, bool);
    vtkSetMacro(ComputeDistanceToLine, bool);
    vtkBooleanMacro(ComputeDistanceToLine, bool);

protected:
    LinearSelectorXY();
    ~LinearSelectorXY() override;

    int FillInputPortInformation(int port, vtkInformation * info) override;
    int FillOutputPortInformation(int port, vtkInformation * info) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    vtkVector2d StartPoint;
    vtkVector2d EndPoint;
    bool OutputPositionOnLine;
    bool ComputeDistanceToLine;
};
