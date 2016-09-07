#pragma once

#include <vtkType.h>


// defined in vtkType.h
#if !defined(VTK_HAS_MTIME_TYPE)
typedef vtkTypeUInt64 vtkMTimeType;
#endif

// defined in vtkConfigure.h
#if !defined(VTK_OVERRIDE)
#define VTK_OVERRIDE override
#endif
#if !defined(VTK_FINAL)
#define VTK_FINAL final
#endif
#if !defined(VTK_DELETE_FUNCTION)
#define VTK_DELETE_FUNCTION =delete
#endif
