#pragma once

#include <core/core_api.h>


class vtkImageData;
class vtkTexture;
class QString;

class CORE_API TextureManager
{
public:
    static void initialize();
    static void release();

    static vtkImageData * imageFromFile(const QString & fileName);
    static vtkTexture * fromFile(const QString & fileName);

private:
    TextureManager() = delete;
};
