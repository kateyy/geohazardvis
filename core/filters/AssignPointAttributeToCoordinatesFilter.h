#pragma once

#include <vtkPointSetAlgorithm.h>
#include <vtkStdString.h>

#include <core/core_api.h>


class CORE_API AssignPointAttributeToCoordinatesFilter : public vtkPointSetAlgorithm
{
public:
    vtkTypeMacro(AssignPointAttributeToCoordinatesFilter, vtkPointSetAlgorithm);
    static AssignPointAttributeToCoordinatesFilter * New();

    vtkGetMacro(AttributeArrayToAssign, const vtkStdString &);
    vtkSetMacro(AttributeArrayToAssign, const vtkStdString &);

protected:
    AssignPointAttributeToCoordinatesFilter();
    ~AssignPointAttributeToCoordinatesFilter() override;

    int ExecuteInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    vtkStdString AttributeArrayToAssign;

private:
    AssignPointAttributeToCoordinatesFilter(const AssignPointAttributeToCoordinatesFilter &) = delete;
    void operator=(const AssignPointAttributeToCoordinatesFilter &) = delete;
};
