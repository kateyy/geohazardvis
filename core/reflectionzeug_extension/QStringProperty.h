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

#include <QString>

#include <reflectionzeug/StringPropertyInterface.h>
#include <reflectionzeug/Property.h>

#include <core/core_api.h>


class CORE_API QStringProperty : public reflectionzeug::ValueProperty<QString, reflectionzeug::StringPropertyInterface>
{
public:
    using Type = QString;

public:
    ~QStringProperty() override = 0;

    void accept(reflectionzeug::AbstractPropertyVisitor * visitor) override;

    std::string toString() const override;
    bool fromString(const std::string & string) override;
};


#include <core/reflectionzeug_extension/QStringProperty.hpp>
