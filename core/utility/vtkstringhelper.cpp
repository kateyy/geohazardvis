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
            static_cast<int>(chars->GetSize()));
    }

    if (auto string = vtkStringArray::SafeDownCast(&data))
    {
        if (string->GetSize() == 0)
        {
            return{};
        }

        return QString::fromStdString(string->GetValue(0));
    }

    return{};
}
