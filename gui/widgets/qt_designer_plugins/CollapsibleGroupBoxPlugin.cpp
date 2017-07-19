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

#include "CollapsibleGroupBoxPlugin.h"

#include "../CollapsibleGroupBox.h"


CollapsibleGroupBoxPlugin::CollapsibleGroupBoxPlugin(QObject * parent)
    : QObject(parent)
    , m_isInitialized{ false }
{
}

bool CollapsibleGroupBoxPlugin::isContainer() const
{
    return true;
}

bool CollapsibleGroupBoxPlugin::isInitialized() const
{
    return m_isInitialized;
}

QIcon CollapsibleGroupBoxPlugin::icon() const
{
    return{};
}

QString CollapsibleGroupBoxPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
        " <widget class=\"CollapsibleGroupBox\" name=\"groupBox\" />\n"
        "</ui>\n";
}

QString CollapsibleGroupBoxPlugin::group() const
{
    return "Containers";
}

QString CollapsibleGroupBoxPlugin::includeFile() const
{
    return "gui/widgets/CollapsibleGroupBox.h";
}

QString CollapsibleGroupBoxPlugin::name() const
{
    return "CollapsibleGroupBox";
}

QString CollapsibleGroupBoxPlugin::toolTip() const
{
    return{};
}

QString CollapsibleGroupBoxPlugin::whatsThis() const
{
    return QString();
}

QWidget * CollapsibleGroupBoxPlugin::createWidget(QWidget * parent)
{
    return new CollapsibleGroupBox(parent);
}

void CollapsibleGroupBoxPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (m_isInitialized)
    {
        return;
    }

    m_isInitialized = true;
}
