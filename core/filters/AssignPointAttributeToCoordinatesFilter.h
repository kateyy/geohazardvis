#pragma once

#include <vtkPointSetAlgorithm.h>
#include <vtkStdString.h>

#include <core/core_api.h>


class CORE_API AssignPointAttributeToCoordinatesFilter : public vtkPointSetAlgorithm
{
public:
    vtkTypeMacro(AssignPointAttributeToCoordinatesFilter, vtkPointSetAlgorithm);
    static AssignPointAttributeToCoordinatesFilter * New();

    /** Name of the point attribute array that is assigned as point coordinates.
      * This may be empty(). In this case, the point coordinates are not changed
      * but provided as point attributes if CurrentCoordinatesAsScalars is set. */
    vtkGetMacro(AttributeArrayToAssign, const vtkStdString &);
    vtkSetMacro(AttributeArrayToAssign, const vtkStdString &);

    /** Assign the current point coordinates also as point scalar attribute. */
    vtkGetMacro(CurrentCoordinatesAsScalars, bool);
    vtkSetMacro(CurrentCoordinatesAsScalars, bool);
    vtkBooleanMacro(CurrentCoordinatesAsScalars, bool);

protected:
    AssignPointAttributeToCoordinatesFilter();
    ~AssignPointAttributeToCoordinatesFilter() override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    vtkStdString AttributeArrayToAssign;
    bool CurrentCoordinatesAsScalars;

private:
    AssignPointAttributeToCoordinatesFilter(const AssignPointAttributeToCoordinatesFilter &) = delete;
    void operator=(const AssignPointAttributeToCoordinatesFilter &) = delete;
};
