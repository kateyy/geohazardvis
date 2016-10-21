#pragma once

#include <cstdint>
#include <utility>
#include <iosfwd>
#include <vector>

#include <QString>

#include <vtkVector.h>

#include <core/CoordinateSystems_fwd.h>


class vtkFieldData;
class vtkInformation;
class vtkInformationDoubleVectorMetaDataKey;
class vtkInformationIntegerMetaDataKey;
class vtkInformationStringMetaDataKey;


struct CORE_API CoordinateSystemType
{
    enum Value : int32_t
    {
        geographic,
        metricGlobal,
        metricLocal,
        other,
        unspecified
    };

    Value value;

    CoordinateSystemType(Value value = unspecified) : value{ value } { }
    explicit CoordinateSystemType(const QString & typeName);

    operator Value() const { return value; }

    const QString & toString() const;
    CoordinateSystemType & fromString(const QString & typeName);

    static const std::vector<std::pair<CoordinateSystemType, QString>> & typeToStringMap();

    friend CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemType & coordsType);
};

CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemType & coordsType);


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
    bool operator!=(const CoordinateSystemSpecification & other) const;
    bool operator==(const ReferencedCoordinateSystemSpecification & referencedSpec) const;
    bool operator!=(const ReferencedCoordinateSystemSpecification & referencedSpec) const;

    virtual void readFromInformation(vtkInformation & information);
    virtual void writeToInformation(vtkInformation & information) const;
    static CoordinateSystemSpecification fromInformation(vtkInformation & information);

    /** vtkDataSet information is not stored in VTK XML files, thus information has to be written to
    the data set's field data for persistent storage. For pipeline requests, however, it is 
    more useful to store the coordinate system in the data set's information, as the will be
    available in pipeline update requests already. */
    virtual void readFromFieldData(vtkFieldData & fieldData);
    virtual void writeToFieldData(vtkFieldData & fieldData) const;
    static ReferencedCoordinateSystemSpecification fromFieldData(vtkFieldData & fieldData);

    static vtkInformationIntegerMetaDataKey * CoordinateSystemType_InfoKey();
    static vtkInformationStringMetaDataKey * GeographicCoordinateSystemName_InfoKey();
    static vtkInformationStringMetaDataKey * MetricCoordinateSystemName_InfoKey();

    friend CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemSpecification & spec);
};

CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemSpecification & spec);


struct CORE_API ReferencedCoordinateSystemSpecification : public CoordinateSystemSpecification
{
    ReferencedCoordinateSystemSpecification();
    ReferencedCoordinateSystemSpecification(
        CoordinateSystemType type,
        const QString & geographicSystem,
        const QString & globalMetricSystem,
        const vtkVector2d & referencePointLatLong,
        const vtkVector2d & referencePointLocalRelative);
    ReferencedCoordinateSystemSpecification(
        const CoordinateSystemSpecification & unreferencedSpec,
        const vtkVector2d & referencePointLatLong,
        const vtkVector2d & referencePointLocalRelative);
    ~ReferencedCoordinateSystemSpecification() override = default;

    bool isReferencePointValid() const;

    bool operator==(const CoordinateSystemSpecification & unreferencedSpec) const;
    bool operator!=(const CoordinateSystemSpecification & unreferencedSpec) const;
    bool operator==(const ReferencedCoordinateSystemSpecification & other) const;
    bool operator!=(const ReferencedCoordinateSystemSpecification & other) const;

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

    void readFromFieldData(vtkFieldData & fieldData) override;
    void writeToFieldData(vtkFieldData & fieldData) const override;
    static ReferencedCoordinateSystemSpecification fromFieldData(vtkFieldData & fieldData);

    static vtkInformationDoubleVectorMetaDataKey * ReferencePointLatLong_InfoKey();
    static vtkInformationDoubleVectorMetaDataKey * ReferencePointLocalRelative_InfoKey();

    friend CORE_API std::ostream & operator<<(std::ostream & os, const ReferencedCoordinateSystemSpecification & spec);
};

CORE_API std::ostream & operator<<(std::ostream & os, const ReferencedCoordinateSystemSpecification & spec);
