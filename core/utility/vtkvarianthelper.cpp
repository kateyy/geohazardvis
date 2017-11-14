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

#include <core/utility/vtkvarianthelper.h>

#include <QDebug>
#include <QString>
#include <QVariant>


namespace
{
    
template<typename UIntType = std::conditional<sizeof(size_t) == sizeof(unsigned int),
    unsigned int, unsigned long long>::type>
UIntType vtkObjectToUIntType(vtkObjectBase * obj)
{
    return reinterpret_cast<UIntType>(obj);
}

}


QVariant vtkVariantToQVariant(const vtkVariant & variant)
{
    switch (variant.GetType())
    {
    case VTK_STRING: return QString::fromStdString(variant.ToString());
    case VTK_UNICODE_STRING: return QString::fromStdString(variant.ToString());
    case VTK_OBJECT: return vtkObjectToUIntType(variant.ToVTKObject());
    case VTK_CHAR: return variant.ToChar();
    case VTK_SIGNED_CHAR: return variant.ToSignedChar();
    case VTK_UNSIGNED_CHAR: return variant.ToUnsignedChar();
    case VTK_SHORT: return variant.ToShort();
    case VTK_UNSIGNED_SHORT: return variant.ToUnsignedShort();
    case VTK_INT: return variant.ToInt();
    case VTK_UNSIGNED_INT: return variant.ToUnsignedInt();
    case VTK_LONG:
        if (sizeof(int) == sizeof(long))
        {
            return variant.ToInt();
        }
        return variant.ToLongLong();
    case VTK_UNSIGNED_LONG:
        if (sizeof(unsigned int) == sizeof(unsigned long))
        {
            return variant.ToUnsignedInt();
        }
        return variant.ToUnsignedLongLong();
    case VTK_LONG_LONG: return variant.ToLongLong();
    case VTK_UNSIGNED_LONG_LONG: return variant.ToUnsignedLongLong();
    case VTK_FLOAT: return variant.ToFloat();
    case VTK_DOUBLE: return variant.ToDouble();
    default:
        qDebug() << "vtkVariantToQVariant: Unhandled vtkVariant type: " << variant.GetType();
        return {};
    }
}
