#pragma once

#include <core/core_api.h>


class vtkTextProperty;


namespace FontHelper
{

CORE_API bool isUnicodeFontAvailable();
CORE_API const char * unicodeFontFileName();

CORE_API bool configureTextProperty(vtkTextProperty & textProperty);

};
