/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vtkDataSetAlgorithm.h>
#include <vtkStdString.h>

#include <core/types.h>


/** Renames current point data scalar array. */
class CORE_API ArrayChangeInformationFilter : public vtkDataSetAlgorithm
{
public:
    static ArrayChangeInformationFilter * New();
    vtkTypeMacro(ArrayChangeInformationFilter, vtkDataSetAlgorithm);

    vtkGetMacro(AttributeLocation, IndexType);
    /** Set the location where to look for the attribute array.
    * This is POINT_DATA by default. */
    vtkSetMacro(AttributeLocation, IndexType);

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

    /** Toggle whether to modify the array unit. */
    vtkBooleanMacro(EnableSetUnit, bool);
    vtkGetMacro(EnableSetUnit, bool);
    vtkSetMacro(EnableSetUnit, bool);

    vtkGetMacro(ArrayUnit, vtkStdString);
    /** Set the vtkDataArray::UNITS_LABEL on the array. */
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
    IndexType AttributeLocation;
    int AttributeType;

    bool EnableRename;
    vtkStdString ArrayName;

    bool EnableSetUnit;
    vtkStdString ArrayUnit;

    bool PassInputArray;

private:
    ArrayChangeInformationFilter(const ArrayChangeInformationFilter&) = delete;
    void operator=(const ArrayChangeInformationFilter&) = delete;
};
