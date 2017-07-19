/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "font.h"

#include <vtkTextProperty.h>

#if WIN32
#include <QFont>
#include <QStandardPaths>
#include <QSettings>
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
    static const auto filePath = [] () -> QString
    {
        auto fileName = [] () -> QString
        {
            const QSettings fontSettings(
                "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                QSettings::NativeFormat);
            const auto defaultFont = QFont().family();
            const auto fontPath = fontSettings.value(defaultFont).toString();
            if (!fontPath.isEmpty())
            {
                return fontPath;
            }
            const QSettings substitutesSettings(
                "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes",
                QSettings::NativeFormat);
            auto substitute = substitutesSettings.value(defaultFont).toString();
            if (substitute.isEmpty())
            {
                return{};
            }
            auto substitutePath = fontSettings.value(substitute).toString();
            if (!substitutePath.isEmpty())
            {
                return substitutePath;
            }
            substitutePath = fontSettings.value(substitute + " (TrueType)").toString();
            if (!substitutePath.isEmpty())
            {
                return substitutePath;
            }
            return{};
        }();

        // Fall-back to a font that should exist on all windows machines
        if (fileName.isEmpty())
        {
            fileName = "arial.ttf";
        }
        auto location = QStandardPaths::locate(QStandardPaths::FontsLocation, fileName);
        if (!location.isEmpty())
        {
            return location;
        }
        return{};
    }().toUtf8();
    return filePath.isEmpty() ? nullptr : filePath.constData();

#elif __linux__
    // On Linux this might be handled within VTK by FreeType/FontConfig
    vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();
    return nullptr;

#else
    return nullptr;
#endif
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
