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

#include <vtkPassThrough.h>

#include <QObject>

#include <core/types.h>


class vtkDataArray;


class CORE_API AttributeArrayModifiedListener : public QObject, public vtkPassThrough
{
    Q_OBJECT

public:
    vtkTypeMacro(AttributeArrayModifiedListener, vtkPassThrough);
    static AttributeArrayModifiedListener * New();

    vtkGetMacro(AttributeLocation, IndexType);
    void SetAttributeLocation(IndexType location);

signals:
    void attributeModified(vtkDataArray * array);

protected:
    AttributeArrayModifiedListener();
    ~AttributeArrayModifiedListener() override;

    int RequestData(
        vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    IndexType AttributeLocation;

    vtkMTimeType LastAttributeMTime;

private:
    AttributeArrayModifiedListener(const AttributeArrayModifiedListener &) = delete;
    void operator=(const AttributeArrayModifiedListener &) = delete;
};
