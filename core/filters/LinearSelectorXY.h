#pragma once

#include <vtkSelectionAlgorithm.h>
#include <vtkVector.h>

#include <core/core_api.h>


class vtkPolyData;


/**
From an input set of cells, extract respective centroids that are located within the rangeof a line segment.

This filter requires an input line segment specified by StartPoint and EndPoint.
An input cell is considered in range, if:
    * Its centroid projected to the line is locate inside the line segment, and
    * There are cell points on both sides of the line (in both half spaces)

Two outputs are produced:
    Port 0: vtkSelection containing relevant cell indices
    Port 1: vtkPolyData containing the centroid of selected cells with additional attributes assigned
*/
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


    enum SortMode
    {
        SortIndices,
        SortPoints,
        SortNone
    };

    /** Set whether output points (port 1) are ordered by their projected position on the extraction line.
        SortIndices: Point indices are sorted. To extract points (and point attributes) in the
            correct order, you have to lookup them using the ordering specified by the indices.
        SortPoints: Points (and point attributes) are sorted. In this case, the point indicies are
            simply in increasing order.
        SortNone: Don't apply any sorting. Extracted points remain in the same order as their
            respective cells in the input data.
    */
    vtkGetMacro(Sorting, SortMode);
    vtkSetClampMacro(Sorting, SortMode, SortIndices, SortNone);
    vtkBooleanMacro(Sorting, SortMode);

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
    SortMode Sorting;
    bool OutputPositionOnLine;
    bool ComputeDistanceToLine;
};
