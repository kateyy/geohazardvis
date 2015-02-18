#pragma once

#include <core/core_api.h>


class QString;
class vtkCharArray;
class vtkDataArray;
template<typename T> class vtkSmartPointer;


vtkSmartPointer<vtkCharArray> CORE_API qstringToVtkArray(const QString & string);
QString CORE_API vtkArrayToQString(vtkDataArray & data);
