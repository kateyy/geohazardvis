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

#include "vtkInformationDoubleVectorMetaDataKey.h"

#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkInformationDoubleVectorMetaDataKey::vtkInformationDoubleVectorMetaDataKey(const char * name, const char * location)
    : vtkInformationDoubleVectorKey(name, location)
{
}

vtkInformationDoubleVectorMetaDataKey::~vtkInformationDoubleVectorMetaDataKey() = default;

vtkInformationDoubleVectorMetaDataKey * vtkInformationDoubleVectorMetaDataKey::MakeKey(const char * name, const char * location)
{
    return new vtkInformationDoubleVectorMetaDataKey(name, location);
}

void vtkInformationDoubleVectorMetaDataKey::CopyDefaultInformation(vtkInformation * request, vtkInformation * fromInfo, vtkInformation * toInfo)
{
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        this->ShallowCopy(fromInfo, toInfo);
    }
}
