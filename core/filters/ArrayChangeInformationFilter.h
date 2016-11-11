#pragma once

#include <vtkDataSetAlgorithm.h>
#include <vtkStdString.h>

#include <core/core_api.h>


/** Renames current point data scalar array. */
class CORE_API ArrayChangeInformationFilter : public vtkDataSetAlgorithm
{
public:
    static ArrayChangeInformationFilter * New();
    vtkTypeMacro(ArrayChangeInformationFilter, vtkDataSetAlgorithm);

    enum AttributeLocations
    {
        POINT_DATA = 0,
        CELL_DATA = 1,
        NUM_ATTRIBUTE_LOCS
    };

    vtkGetMacro(AttributeLocation, AttributeLocations);
    /** Set the location where to look for the attribute array.
    * This is POINT_DATA by default. */
    vtkSetMacro(AttributeLocation, AttributeLocations);

    vtkGetMacro(AttributeType, int);
    /** Set the attribute array to be modified.
    * This value must be one of vtkDataSetAttributes::AttributeTypes and is
    * vtkDataSetAttributes::SCALARS by default. */
    vtkSetMacro(AttributeType, int);

    /** Toggle whether to modify the array name. */
    vtkBooleanMacro(EnableRename, bool);
    vtkGetMacro(EnableRename, bool);
    vtkSetMacro(EnableRename, bool);

    vtkGetMacro(ArrayName, vtkStdString);
    vtkSetMacro(ArrayName, vtkStdString);

    /** Toggle whether to modify the array unit. Requires VTK version 7.1.0 or newer */
    vtkBooleanMacro(EnableSetUnit, bool);
    vtkGetMacro(EnableSetUnit, bool);
    vtkSetMacro(EnableSetUnit, bool);

    vtkGetMacro(ArrayUnit, vtkStdString);
    /** Set the vtkDataArray::UNITS_LABEL on the array. This requires VTK version 7.1.0 or newer */
    vtkSetMacro(ArrayUnit, vtkStdString);


    /** Also pass the input array to the output.
    * By default, the input array is copied (and information are changed), but not passed to the output.
    * This does only make sense if renaming is enabled, since data set attributes to not meant to
    * be used with multiple arrays with same names. */
    vtkBooleanMacro(PassInputArray, bool);
    vtkGetMacro(PassInputArray, bool);
    vtkSetMacro(PassInputArray, bool);

protected:
    ArrayChangeInformationFilter();
    ~ArrayChangeInformationFilter() override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    ArrayChangeInformationFilter(const ArrayChangeInformationFilter&) = delete;
    void operator=(const ArrayChangeInformationFilter&) = delete;

private:
    AttributeLocations AttributeLocation;
    int AttributeType;

    bool EnableRename;
    vtkStdString ArrayName;

    bool EnableSetUnit;
    vtkStdString ArrayUnit;

    bool PassInputArray;
};
