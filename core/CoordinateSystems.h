#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <QString>

#include <vtkVector.h>

#include <core/core_api.h>


class vtkInformation;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationStringKey;


struct CORE_API CoordinateSystemType
{
    enum Value : int32_t
    {
        geographic,
        metricGlobal,
        metricLocal,
        unspecified
    };

    Value value;

    CoordinateSystemType(Value value = unspecified) : value{ value } { }
    explicit CoordinateSystemType(const QString & typeName);

    operator Value() const { return value; }

    const QString & toString() const;
    CoordinateSystemType & fromString(const QString & typeName);

    static const std::vector<std::pair<CoordinateSystemType, QString>> & typeToStringMap();
};


struct ReferencedCoordinateSystemSpecification;

struct CORE_API CoordinateSystemSpecification
{
    CoordinateSystemSpecification();
    CoordinateSystemSpecification(
        CoordinateSystemType type,
        const QString & geographicSystem,
        const QString & globalMetricSystem);

    virtual ~CoordinateSystemSpecification() = default;
        
    /** Type of the specified coordinate system */
    CoordinateSystemType type;
    /** Specification for latitude and longitude coordinates, e.g., WGS84 */
    QString geographicSystem;
    /** Metric reference system, e.g., UTM
    This must not be set if type is set to CoordinateSystemType::geographic. */
    QString globalMetricSystem;

    bool isValid(bool allowUnspecified) const;

    bool operator==(const CoordinateSystemSpecification & other) const;
    bool operator==(const ReferencedCoordinateSystemSpecification & referencedSpec) const;

    virtual void readFromInformation(vtkInformation & information);
    virtual void writeToInformation(vtkInformation & information) const;
    static CoordinateSystemSpecification fromInformation(vtkInformation & information);

    static vtkInformationIntegerKey * CoordinateSystemType_InfoKey();
    static vtkInformationStringKey * GeographicCoordinateSystemName_InfoKey();
    static vtkInformationStringKey * MetricCoordinateSystemName_InfoKey();
};


struct CORE_API ReferencedCoordinateSystemSpecification : public CoordinateSystemSpecification
{
    ReferencedCoordinateSystemSpecification();
    ReferencedCoordinateSystemSpecification(
        CoordinateSystemType type,
        const QString & geographicSystem,
        const QString & globalMetricSystem,
        const vtkVector2d & referencePointLatLong,
        const vtkVector2d & referencePointLocalRelative);
    ~ReferencedCoordinateSystemSpecification() override = default;

    bool isReferencePointValid() const;

    bool operator==(const CoordinateSystemSpecification & unreferencedSpec) const;
    bool operator==(const ReferencedCoordinateSystemSpecification & other) const;

    /** The reference point is required to convert between global and local coordinates.
    This parameter sets latitude and longitude of the reference point in the coordinate system
    defined by geographicCoordinateSystem. */
    vtkVector2d referencePointLatLong;
    /** Relative position of the reference point within the data set bounds.
    * X is the easting, Y is the northing of the data set*/
    vtkVector2d referencePointLocalRelative;


    void readFromInformation(vtkInformation & information) override;
    void writeToInformation(vtkInformation & information) const override;
    static ReferencedCoordinateSystemSpecification fromInformation(vtkInformation & information);

    static vtkInformationDoubleVectorKey * ReferencePointLatLong_InfoKey();
    static vtkInformationDoubleVectorKey * ReferencePointLocalRelative_InfoKey();
};


struct ConversionCheckResult
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
