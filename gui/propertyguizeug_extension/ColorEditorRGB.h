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

#pragma once

#include <array>

#include <reflectionzeug/property_declaration.h>
#include <propertyguizeug/PropertyEditor.h>

#include <gui/gui_api.h>


class QSpinBox;

namespace reflectionzeug
{
    class ColorPropertyInterface;
    class Color;
}
class ColorButtonWithBorder;


class GUI_API ColorEditorRGB : public propertyguizeug::PropertyEditor
{
public:
    using Type = reflectionzeug::ColorPropertyInterface;

    static void paint(QPainter * painter,
        const QStyleOptionViewItem & option,
        reflectionzeug::ColorPropertyInterface & property);

    ColorEditorRGB(reflectionzeug::ColorPropertyInterface * property, QWidget * parent = nullptr);
    ~ColorEditorRGB() override;

    void openColorPicker();
    void readUiColor(int);

protected:
    QColor qColor() const;
    void setQColor(const QColor & qcolor);
    void setColor(const reflectionzeug::Color & color);

protected:
    ColorButtonWithBorder * m_button;
    std::array<QSpinBox *, 3> m_spinBoxes;

    reflectionzeug::ColorPropertyInterface * m_property;

private:
    Q_DISABLE_COPY(ColorEditorRGB)
};
