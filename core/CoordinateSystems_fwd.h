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

#include <core/core_api.h>


struct CoordinateSystemType;
struct CoordinateSystemSpecification;
struct ReferencedCoordinateSystemSpecification;


struct CORE_API ConversionCheckResult
{
    enum class Result
    {
        okay, missingInformation, unsupported, invalidParameters
    };

    explicit ConversionCheckResult(Result result) : result{ result } { }

    static ConversionCheckResult okay()
        { return ConversionCheckResult(Result::okay); };
    static ConversionCheckResult missingInformation()
        { return ConversionCheckResult(Result::missingInformation); };
    static ConversionCheckResult unsupported()
        { return ConversionCheckResult(Result::unsupported); };
    static ConversionCheckResult invalidParameters()
        { return ConversionCheckResult(Result::invalidParameters); };

    operator bool() const
    {
        return result == Result::okay;
    }

    const Result result;
};
