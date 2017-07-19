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

#include <cstdint>
#include <utility>
#include <iosfwd>
#include <vector>

#include <QMetaType>
#include <QString>

#include <vtkVector.h>

#include <core/CoordinateSystems_fwd.h>


class QDataStream;
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

    CoordinateSystemType(const CoordinateSystemType &) = default;
    CoordinateSystemType(CoordinateSystemType &&) = default;
    CoordinateSystemType & operator=(const CoordinateSystemType &) = default;
    CoordinateSystemType & operator=(CoordinateSystemType &&) = default;
};
Q_DECLARE_METATYPE(CoordinateSystemType)
CORE_API QDataStream & operator<<(QDataStream & stream, const CoordinateSystemType & coordsType);
CORE_API QDataStream & operator>>(QDataStream & stream, CoordinateSystemType & coordsType);

CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemType & coordsType);


struct CORE_API CoordinateSystemSpecification
{
    CoordinateSystemSpecification();
    CoordinateSystemSpecification(
        CoordinateSystemType type,
        const QString & geographicSystem,
        const QString & globalMetricSystem,
        const QString & unitOfMeasurement
    );

    virtual ~CoordinateSystemSpecification() = default;

    /** Type of the specified coordinate system */
    CoordinateSystemType type;
    /** Specification for latitude and longitude coordinates, e.g., WGS 84 */
    QString geographicSystem;
    /**
     * Metric reference system, e.g., UTM
     * This must not be set if type is set to CoordinateSystemType::geographic.
     */
    QString globalMetricSystem;
    /** 
     * Unit of the coordinates, e.g., m or km for metric coordinates.
     * For geographic coordinate systems, this is assumed to be degrees and ignored in most places.
     */
    QString unitOfMeasurement;

    /**
     * Check whether this specification is not CoordinateSystemType::unspecified and at least all
     * required parameters for the current type are set.
     * geographic -> requires geographicSystem (unit is generally degree)
     * metricGlobal/Local -> additionally require globalMetricSystem and coordinatesUnit
     */
    bool isValid() const;
    /** 
     * Check whether type is unspecified and no further parameters are set. This allows to detect
     * incomplete specifications.
     */
    bool isUnspecified() const;

    bool operator==(const CoordinateSystemSpecification & other) const;
    bool operator!=(const CoordinateSystemSpecification & other) const;
    bool operator==(const ReferencedCoordinateSystemSpecification & referencedSpec) const;
    bool operator!=(const ReferencedCoordinateSystemSpecification & referencedSpec) const;

    virtual void readFromInformation(vtkInformation & information);
    virtual void writeToInformation(vtkInformation & information) const;
    static CoordinateSystemSpecification fromInformation(vtkInformation & information);

    /**
     * vtkDataSet information is not stored in VTK XML files, thus information has to be written
     * to the data set's field data for persistent storage. For pipeline requests, however, it is
     * more useful to store the coordinate system in the data set's information, as the will be
     * available in pipeline update requests already.
     */
    virtual void readFromFieldData(vtkFieldData & fieldData);
    virtual void writeToFieldData(vtkFieldData & fieldData) const;
    static ReferencedCoordinateSystemSpecification fromFieldData(vtkFieldData & fieldData);

    static vtkInformationIntegerMetaDataKey * CoordinateSystemType_InfoKey();
    static vtkInformationStringMetaDataKey * GeographicCoordinateSystemName_InfoKey();
    static vtkInformationStringMetaDataKey * MetricCoordinateSystemName_InfoKey();
    static vtkInformationStringMetaDataKey * UnitOfMeasurement_InfoKey();

    CoordinateSystemSpecification(const CoordinateSystemSpecification &) = default;
    CoordinateSystemSpecification(CoordinateSystemSpecification &&) = default;
    CoordinateSystemSpecification & operator=(const CoordinateSystemSpecification &) = default;
    CoordinateSystemSpecification & operator=(CoordinateSystemSpecification &&) = default;

    friend CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemSpecification & spec);
};
Q_DECLARE_METATYPE(CoordinateSystemSpecification)
CORE_API QDataStream & operator<<(QDataStream & stream, const CoordinateSystemSpecification & spec);
CORE_API QDataStream & operator>>(QDataStream & stream, CoordinateSystemSpecification & spec);

CORE_API std::ostream & operator<<(std::ostream & os, const CoordinateSystemSpecification & spec);


struct CORE_API ReferencedCoordinateSystemSpecification : public CoordinateSystemSpecification
{
    ReferencedCoordinateSystemSpecification();
    ReferencedCoordinateSystemSpecification(
        CoordinateSystemType type,
        const QString & geographicSystem,
        const QString & globalMetricSystem,
        const QString & unitOfMeasurement,
        vtkVector2d referencePointLatLong);
    ReferencedCoordinateSystemSpecification(
        CoordinateSystemSpecification unreferencedSpec,
        vtkVector2d referencePointLatLong);
    ~ReferencedCoordinateSystemSpecification() override = default;

    bool isReferencePointValid() const;

    bool operator==(const CoordinateSystemSpecification & unreferencedSpec) const;
    bool operator!=(const CoordinateSystemSpecification & unreferencedSpec) const;
    bool operator==(const ReferencedCoordinateSystemSpecification & other) const;
    bool operator!=(const ReferencedCoordinateSystemSpecification & other) const;

    /**
     * The reference point is required to convert between global and local coordinates.
     * This parameter sets latitude and longitude of the reference point in the coordinate system
     * defined by geographicCoordinateSystem.
     */
    vtkVector2d referencePointLatLong;


    void readFromInformation(vtkInformation & information) override;
    void writeToInformation(vtkInformation & information) const override;
    static ReferencedCoordinateSystemSpecification fromInformation(vtkInformation & information);

    void readFromFieldData(vtkFieldData & fieldData) override;
    void writeToFieldData(vtkFieldData & fieldData) const override;
    static ReferencedCoordinateSystemSpecification fromFieldData(vtkFieldData & fieldData);

    static vtkInformationDoubleVectorMetaDataKey * ReferencePointLatLong_InfoKey();
    static vtkInformationDoubleVectorMetaDataKey * ReferencePointLocalRelative_InfoKey();

    ReferencedCoordinateSystemSpecification(const ReferencedCoordinateSystemSpecification &) = default;
    ReferencedCoordinateSystemSpecification(ReferencedCoordinateSystemSpecification &&) = default;
    ReferencedCoordinateSystemSpecification & operator=(const ReferencedCoordinateSystemSpecification &) = default;
    ReferencedCoordinateSystemSpecification & operator=(ReferencedCoordinateSystemSpecification &&) = default;

    friend CORE_API std::ostream & operator<<(std::ostream & os, const ReferencedCoordinateSystemSpecification & spec);
};
Q_DECLARE_METATYPE(ReferencedCoordinateSystemSpecification)
CORE_API QDataStream & operator<<(QDataStream & stream, const ReferencedCoordinateSystemSpecification & spec);
CORE_API QDataStream & operator>>(QDataStream & stream, ReferencedCoordinateSystemSpecification & spec);

CORE_API std::ostream & operator<<(std::ostream & os, const ReferencedCoordinateSystemSpecification & spec);
