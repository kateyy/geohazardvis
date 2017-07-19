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

#include <core/utility/vtkstringhelper.h>

#include <QString>

#include <vtkCharArray.h>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>


vtkSmartPointer<vtkCharArray> qstringToVtkArray(const QString & string)
{
    auto array = vtkSmartPointer<vtkCharArray>::New();

    qstringToVtkArray(string, *array);

    return array;
}

void qstringToVtkArray(const QString & string, vtkCharArray & array)
{
    const auto bytes = string.toUtf8();

    array.SetNumberOfValues(bytes.size());

    for (int i = 0; i < bytes.size(); ++i)
    {
        array.SetValue(i, bytes.at(i));
    }
}

QString vtkArrayToQString(vtkAbstractArray & data)
{
    if (auto chars = vtkCharArray::FastDownCast(&data))
    {
        return QString::fromUtf8(
            chars->GetPointer(0),
            static_cast<int>(chars->GetNumberOfValues()));
    }

    if (auto string = vtkStringArray::SafeDownCast(&data))
    {
        if (string->GetNumberOfValues() == 0)
        {
            return{};
        }

        return QString::fromStdString(string->GetValue(0));
    }

    return{};
}
