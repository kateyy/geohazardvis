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

#include <core/reflectionzeug_extension/QStringProperty.h>

#include <reflectionzeug/PropertyVisitor.h>


QStringProperty::~QStringProperty() = default;

void QStringProperty::accept(reflectionzeug::AbstractPropertyVisitor * visitor)
{
    reflectionzeug::StringPropertyInterface::accept(visitor);
}

std::string QStringProperty::toString() const
{
    return std::string{ this->value().toUtf8().data() };
}

bool QStringProperty::fromString(const std::string & string)
{
    QString value = QString::fromUtf8(string.c_str());

    this->setValue(value);
    return true;
}
