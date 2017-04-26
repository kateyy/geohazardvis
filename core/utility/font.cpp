#include "font.h"

#include <vtkTextProperty.h>

#if WIN32
#include <QStandardPaths>
#elif __linux__
#include <vtkFreeTypeTools.h>
#endif


namespace FontHelper
{

bool isUnicodeFontAvailable()
{
    static const bool available = unicodeFontFileName() != nullptr && unicodeFontFileName()[0] != 0;
    return available;
}

const char * unicodeFontFileName()
{
#if WIN32
    static const auto fileName =
        QStandardPaths::locate(QStandardPaths::FontsLocation, "arial.ttf").toUtf8();
    return fileName.constData();

#elif __linux__
    // On Linux this might be handled within VTK by FreeType/FontConfig
    vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();
    return nullptr;

#endif
    return nullptr;
}

bool configureTextProperty(vtkTextProperty & textProperty)
{
    if (!isUnicodeFontAvailable())
    {
        return false;
    }

    textProperty.SetFontFamily(VTK_FONT_FILE);
    textProperty.SetFontFile(unicodeFontFileName());

    return true;
}

}
