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
    case VTK_OBJECT: vtkObjectToUIntType(variant.ToVTKObject());
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
