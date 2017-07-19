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

#include <vtkInformationDoubleVectorKey.h>

#include <core/core_api.h>

class CORE_API vtkInformationDoubleVectorMetaDataKey : public vtkInformationDoubleVectorKey
{
public:
    vtkTypeMacro(vtkInformationDoubleVectorMetaDataKey, vtkInformationDoubleVectorKey);

    vtkInformationDoubleVectorMetaDataKey(const char * name, const char * location);
    ~vtkInformationDoubleVectorMetaDataKey() override;

    static vtkInformationDoubleVectorMetaDataKey * MakeKey(const char * name, const char * location);

    /* Shallow copies the key from fromInfo to toInfo if request has the REQUEST_INFORMATION() key.
    * This is used by the pipeline to propagate this key downstream. */
    void CopyDefaultInformation(vtkInformation * request,
        vtkInformation * fromInfo, vtkInformation * toInfo) override;

private:
    vtkInformationDoubleVectorMetaDataKey(const vtkInformationDoubleVectorMetaDataKey &) = delete;
    void operator=(const vtkInformationDoubleVectorMetaDataKey &) = delete;
};
