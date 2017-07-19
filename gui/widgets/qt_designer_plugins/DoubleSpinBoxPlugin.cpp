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

#include "DoubleSpinBoxPlugin.h"

#include "../DoubleSpinBox.h"


DoubleSpinBoxPlugin::DoubleSpinBoxPlugin(QObject * parent)
    : QObject(parent)
    , m_isInitialized{ false }
{
}

bool DoubleSpinBoxPlugin::isContainer() const
{
    return false;
}

bool DoubleSpinBoxPlugin::isInitialized() const
{
    return m_isInitialized;
}

QIcon DoubleSpinBoxPlugin::icon() const
{
    return{};
}

QString DoubleSpinBoxPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
        " <widget class=\"DoubleSpinBox\" name=\"doubleSpinBox\" />\n"
        "</ui>\n";
}

QString DoubleSpinBoxPlugin::group() const
{
    return "Input Widgets";
}

QString DoubleSpinBoxPlugin::includeFile() const
{
    return "gui/widgets/DoubleSpinBox.h";
}

QString DoubleSpinBoxPlugin::name() const
{
    return "DoubleSpinBox";
}

QString DoubleSpinBoxPlugin::toolTip() const
{
    return{};
}

QString DoubleSpinBoxPlugin::whatsThis() const
{
    return QString();
}

QWidget * DoubleSpinBoxPlugin::createWidget(QWidget * parent)
{
    return new DoubleSpinBox(parent);
}

void DoubleSpinBoxPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (m_isInitialized)
    {
        return;
    }

    m_isInitialized = true;
}
