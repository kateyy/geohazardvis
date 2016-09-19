#include "CoordinateSystems.h"

#include <algorithm>
#include <type_traits>

#include <vtkInformation.h>
#include <vtkInformationDoubleVectorKey.h>
#include <vtkInformationIntegerKey.h>
#include <vtkInformationStringKey.h>

#include <core/utility/vtkvectorhelper.h>


vtkInformationKeyMacro(CoordinateSystemSpecification, CoordinateSystemType_InfoKey, Integer);
vtkInformationKeyMacro(CoordinateSystemSpecification, GeographicCoordinateSystemName_InfoKey, String);
vtkInformationKeyMacro(CoordinateSystemSpecification, MetricCoordinateSystemName_InfoKey, String);
vtkInformationKeyMacro(ReferencedCoordinateSystemSpecification, ReferencePointLatLong_InfoKey, DoubleVector);
vtkInformationKeyMacro(ReferencedCoordinateSystemSpecification, ReferencePointLocalRelative_InfoKey, DoubleVector);


namespace
{

const QString & coordinateSytemTypeStringUnkown()
{
    static const QString unkown = "(unspecified)";
    return unkown;
}

}

const std::vector<std::pair<CoordinateSystemType, QString>> & CoordinateSystemType::typeToStringMap()
{
    static const auto map = std::vector<std::pair<CoordinateSystemType, QString>>({
        { CoordinateSystemType::geographic, "geographic" },
        { CoordinateSystemType::metricGlobal, "metric (global)" },
        { CoordinateSystemType::metricLocal, "metric (local)" },
        { CoordinateSystemType::unspecified, coordinateSytemTypeStringUnkown() }
    });
    return map;
}

CoordinateSystemType::CoordinateSystemType(const QString & typeName)
    : value{ CoordinateSystemType::fromString(typeName) }
{
}

const QString & CoordinateSystemType::toString() const
{
    const auto it = std::find_if(typeToStringMap().cbegin(), typeToStringMap().cend(),
        [this] (const std::pair<CoordinateSystemType, QString> & pair) {
        return pair.first == this->value;
    });

    if (it != typeToStringMap().cend())
    {
        return it->second;
    }

    return coordinateSytemTypeStringUnkown();
}

CoordinateSystemType & CoordinateSystemType::fromString(const QString & typeName)
{
    const auto it = std::find_if(typeToStringMap().cbegin(), typeToStringMap().cend(),
        [&typeName] (const std::pair<CoordinateSystemType, QString> & pair) {
        return pair.second == typeName;
    });

    if (it != typeToStringMap().cend())
    {
        value = it->first;
    }
    else
    {
        value = Value::unspecified;
    }

    return *this;
}

CoordinateSystemSpecification::CoordinateSystemSpecification()
    : type{ CoordinateSystemType::unspecified }
    , geographicSystem{}
    , globalMetricSystem{}
{
}

CoordinateSystemSpecification::CoordinateSystemSpecification(
    CoordinateSystemType type,
    const QString & geographicSystem,
    const QString & globalMetricSystem)
    : type{ type }
    , geographicSystem{ geographicSystem }
    , globalMetricSystem{ globalMetricSystem }
{
}

bool CoordinateSystemSpecification::isValid(bool allowUnspecified) const
{
    if (type == CoordinateSystemType::unspecified)
    {
        if (!allowUnspecified)
        {
            return false;
        }

        return geographicSystem.isEmpty() && globalMetricSystem.isEmpty();
    }

    if (type == CoordinateSystemType::geographic
        && geographicSystem.isEmpty())
    {
        return false;
    }

    if (type != CoordinateSystemType::geographic
        && (geographicSystem.isEmpty()
            || globalMetricSystem.isEmpty()))
    {
        return false;
    }

    return true;
}

bool CoordinateSystemSpecification::operator==(const CoordinateSystemSpecification & other) const
{
    return type == other.type
        && geographicSystem == other.geographicSystem
        && globalMetricSystem == other.globalMetricSystem;
}

bool CoordinateSystemSpecification::operator==(const ReferencedCoordinateSystemSpecification & referencedSpec) const
{
    return referencedSpec == *this;
}

void CoordinateSystemSpecification::readFromInformation(vtkInformation & info)
{
    *this = CoordinateSystemSpecification();

    if (info.Has(CoordinateSystemType_InfoKey()))
    {
        type = static_cast<CoordinateSystemType::Value>(info.Get(CoordinateSystemType_InfoKey()));
    }
    if (info.Has(GeographicCoordinateSystemName_InfoKey()))
    {
        geographicSystem = QString::fromUtf8(info.Get(GeographicCoordinateSystemName_InfoKey()));
    }
    if (info.Has(MetricCoordinateSystemName_InfoKey()))
    {
        globalMetricSystem = QString::fromUtf8(info.Get(MetricCoordinateSystemName_InfoKey()));
    }
}

void CoordinateSystemSpecification::writeToInformation(vtkInformation & info) const
{
    if (type != CoordinateSystemType::unspecified)
    {
        info.Set(CoordinateSystemType_InfoKey(), static_cast<std::underlying_type_t<decltype(type)::Value>>(type));
    }
    else
    {
        info.Remove(CoordinateSystemType_InfoKey());
    }
    if (!geographicSystem.isEmpty())
    {
        info.Set(GeographicCoordinateSystemName_InfoKey(), geographicSystem.toUtf8().data());
    }
    else
    {
        info.Remove(GeographicCoordinateSystemName_InfoKey());
    }
    if (!globalMetricSystem.isEmpty())
    {
        info.Set(MetricCoordinateSystemName_InfoKey(), globalMetricSystem.toUtf8().data());
    }
    else
    {
        info.Remove(MetricCoordinateSystemName_InfoKey());
    }
}

CoordinateSystemSpecification CoordinateSystemSpecification::fromInformation(vtkInformation & information)
{
    CoordinateSystemSpecification spec;
    spec.readFromInformation(information);
    return spec;
}

ReferencedCoordinateSystemSpecification::ReferencedCoordinateSystemSpecification()
    : CoordinateSystemSpecification()
{
    uninitializeVector(referencePointLatLong);
    uninitializeVector(referencePointLocalRelative);
}

ReferencedCoordinateSystemSpecification::ReferencedCoordinateSystemSpecification(
    CoordinateSystemType type,
    const QString & geographicSystem,
    const QString & globalMetricSystem,
    const vtkVector2d & referencePointLatLong,
    const vtkVector2d & referencePointLocalRelative)
    : CoordinateSystemSpecification(type, geographicSystem, globalMetricSystem)
    , referencePointLatLong{ referencePointLatLong }
    , referencePointLocalRelative{ referencePointLocalRelative }
{
}

bool ReferencedCoordinateSystemSpecification::isReferencePointValid() const
{
    return isVectorInitialized(referencePointLatLong)
        && isVectorInitialized((referencePointLocalRelative));
}

bool ReferencedCoordinateSystemSpecification::operator==(const CoordinateSystemSpecification & unreferencedSpec) const
{
    return CoordinateSystemSpecification::operator==(unreferencedSpec);
}

bool ReferencedCoordinateSystemSpecification::operator==(const ReferencedCoordinateSystemSpecification & other) const
{
    return CoordinateSystemSpecification::operator==(static_cast<const CoordinateSystemSpecification &>(other))
        && referencePointLatLong == other.referencePointLatLong
        && referencePointLocalRelative == other.referencePointLocalRelative;
}

void ReferencedCoordinateSystemSpecification::readFromInformation(vtkInformation & info)
{
    CoordinateSystemSpecification::readFromInformation(info);

    uninitializeVector(referencePointLatLong);
    uninitializeVector(referencePointLocalRelative);

    if (info.Has(ReferencePointLatLong_InfoKey()))
    {
        referencePointLatLong = vtkVector2d(info.Get(ReferencePointLatLong_InfoKey()));
    }
    if (info.Has(ReferencePointLocalRelative_InfoKey()))
    {
        referencePointLocalRelative = vtkVector2d(info.Get(ReferencePointLocalRelative_InfoKey()));
    }
}

void ReferencedCoordinateSystemSpecification::writeToInformation(vtkInformation & info) const
{
    CoordinateSystemSpecification::writeToInformation(info);

    if (isVectorInitialized(referencePointLatLong))
    {
        info.Set(ReferencePointLatLong_InfoKey(), referencePointLatLong.GetData(), referencePointLatLong.GetSize());
    }
    else
    {
        info.Remove(ReferencePointLatLong_InfoKey());
    }
    if (isVectorInitialized(referencePointLocalRelative))
    {
        info.Set(ReferencePointLocalRelative_InfoKey(), referencePointLocalRelative.GetData(), referencePointLocalRelative.GetSize());
    }
    else
    {
        info.Remove(ReferencePointLocalRelative_InfoKey());
    }
}

ReferencedCoordinateSystemSpecification ReferencedCoordinateSystemSpecification::fromInformation(vtkInformation & information)
{
    ReferencedCoordinateSystemSpecification spec;
    spec.readFromInformation(information);
    return spec;
}
