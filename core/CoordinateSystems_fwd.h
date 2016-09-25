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
