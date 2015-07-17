#include <core/utility/vtkstringhelper.h>

#include <QString>

#include <vtkCharArray.h>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>


vtkSmartPointer<vtkCharArray> qstringToVtkArray(const QString & string)
{
    auto bytes = string.toUtf8();

    auto array = vtkSmartPointer<vtkCharArray>::New();
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
            static_cast<int>(chars->GetSize()));
    }

    if (vtkStringArray * string = vtkStringArray::SafeDownCast(&data))
    {
        if (string->GetSize() == 0)
            return{};

        return QString::fromStdString(string->GetValue(0));
    }

    return{};
}
