#include "font.h"

#include <QStandardPaths>

#include <vtkTextProperty.h>


namespace FontHelper
{

bool isUnicodeFontAvailable()
{
    return unicodeFontFileName() != nullptr;
}

const char * unicodeFontFileName()
{
    static const auto fileName =
        QStandardPaths::locate(QStandardPaths::FontsLocation, "arial.ttf").toUtf8();

    return fileName.constData();
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
