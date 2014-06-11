#pragma once

#include <vtkSmartPointer.h>


#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
