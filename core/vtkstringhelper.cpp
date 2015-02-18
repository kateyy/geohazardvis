#include <core/vtkstringhelper.h>

#include <QString>

#include <vtkCharArray.h>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>

#include <core/vtkhelper.h>


vtkSmartPointer<vtkCharArray> qstringToVtkArray(const QString & string)
{
    auto bytes = string.toUtf8();

    VTK_CREATE(vtkCharArray, array);
    array->SetNumberOfValues(bytes.size());

    for (int i = 0; i < bytes.size(); ++i)
    {
        array->SetValue(i, bytes.at(i));
    }

    return array;
}

QString vtkArrayToQString(vtkDataArray & data)
{
    if (vtkCharArray * chars = vtkCharArray::SafeDownCast(&data))
    {
        return QString::fromUtf8(
            chars->GetPointer(0),
            chars->GetSize());
    }
    
    if (vtkStringArray * string = vtkStringArray::SafeDownCast(&data))
    {
        if (string->GetSize() == 0)
            return{};

        return QString::fromStdString(string->GetValue(0));
    }

    return{};
}
