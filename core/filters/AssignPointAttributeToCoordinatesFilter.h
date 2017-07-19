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
