#include "CoordinateSystems.h"

#include <algorithm>
#include <ostream>
#include <type_traits>

#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkVariantCast.h>

#include <core/filters/vtkInformationDoubleVectorMetaDataKey.h>
#include <core/filters/vtkInformationIntegerMetaDataKey.h>
#include <core/filters/vtkInformationStringMetaDataKey.h>
#include <core/utility/vtkstringhelper.h>
#include <core/utility/vtkvectorhelper.h>


vtkInformationKeyMacro(CoordinateSystemSpecification, CoordinateSystemType_InfoKey, IntegerMetaData);
vtkInformationKeyMacro(CoordinateSystemSpecification, GeographicCoordinateSystemName_InfoKey, StringMetaData);
vtkInformationKeyMacro(CoordinateSystemSpecification, MetricCoordinateSystemName_InfoKey, StringMetaData);
vtkInformationKeyMacro(ReferencedCoordinateSystemSpecification, ReferencePointLatLong_InfoKey, DoubleVectorMetaData);
vtkInformationKeyMacro(ReferencedCoordinateSystemSpecification, ReferencePointLocalRelative_InfoKey, DoubleVectorMetaData);


namespace
{

const QString & coordinateSytemTypeStringUnkown()
{
    static const QString unkown = "(unspecified)";
    return unkown;
}

const char * arrayName_type()
{
    static const char * const name = "CoordinateSystem_type";
    return name;
}

const char * arrayName_geographicSystem()
{
    static const char * const name = "CoordinateSystem_GeographicSystem";
    return name;
}

const char * arrayName_metricSystem()
{
    static const char * const name = "CoordinateSystem_MetricSystem";
    return name;
}

const char * arrayName_geoReference()
{
    static const char * const name = "CoordinateSystem_GeoReferencePoint";
    return name;
}

const char * arrayName_relativeReference()
{
    static const char * const name = "CoordinateSystem_LocalRelativeReferencePoint";
    return name;
}


template<typename ValueType, typename = std::enable_if_t<std::is_arithmetic<ValueType>::value>>
ValueType vtkVariantToValue(const vtkVariant & variant, bool * isValid = nullptr)
{
    return vtkVariantCast<ValueType>(variant, isValid);
}

template<typename ValueType>
std::enable_if_t<std::is_enum<ValueType>::value, ValueType>
vtkVariantToValue(const vtkVariant & variant, bool * isValid = nullptr)
{
    using underlying_t = std::underlying_type_t<ValueType>;
    return static_cast<ValueType>(vtkVariantToValue<underlying_t>(variant, isValid));
};


template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
int vtkTypeForValueType();

template<> int vtkTypeForValueType<int32_t>()
{
    return VTK_TYPE_INT32;
}
template<> int vtkTypeForValueType<int64_t>()
{
    return VTK_TYPE_INT64;
}
template<> int vtkTypeForValueType<uint32_t>()
{
    return VTK_TYPE_UINT32;
}
template<> int vtkTypeForValueType<uint64_t>()
{
    return VTK_TYPE_UINT64;
}
template<> int vtkTypeForValueType<double>()
{
    return VTK_DOUBLE;
}
template<> int vtkTypeForValueType<char>()
{
    return VTK_CHAR;
}
template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>, typename Underlying = std::underlying_type_t<T>>
int vtkTypeForValueType()
{
    return vtkTypeForValueType<Underlying>();
};

template<typename ValueType, int NumComponents = 1, typename = std::enable_if_t<!std::is_enum<ValueType>::value>>
void checkSetArray(vtkFieldData & fieldData, const vtkTuple<ValueType, NumComponents> & value,
    const vtkTuple<ValueType, NumComponents> & nullValue, const char * arrayName)
{
    int arrayIndex;
    vtkSmartPointer<vtkAbstractArray> abstractArray = fieldData.GetAbstractArray(arrayName, arrayIndex);
    const int vtkType = vtkTypeForValueType<ValueType>();
    if (abstractArray && abstractArray->GetDataType() != vtkType)
    {
        fieldData.RemoveArray(arrayIndex);
        abstractArray = nullptr;
    }

    if (value != nullValue)
    {
        if (!abstractArray)
        {
            abstractArray.TakeReference(vtkDataArray::CreateDataArray(vtkType));
            abstractArray->SetName(arrayName);
            fieldData.AddArray(abstractArray);
        }
        abstractArray->SetNumberOfComponents(NumComponents);
        abstractArray->SetNumberOfTuples(1);
        for (int i = 0; i < NumComponents; ++i)
        {
            abstractArray->SetVariantValue(i, value[i]);
        }
    }
    else if (abstractArray)
    {
        fieldData.RemoveArray(arrayIndex);
    }
}

template<typename ValueType, int NumComponents = 1, typename = std::enable_if_t<std::is_enum<ValueType>::value>,
    typename Underlying = std::underlying_type_t<ValueType>>
void checkSetArray(vtkFieldData & fieldData, const vtkTuple<ValueType, NumComponents> & value,
    const vtkTuple<ValueType, NumComponents> & nullValue, const char * arrayName)
{
    return checkSetArray(fieldData,
        reinterpret_cast<const vtkTuple<Underlying, NumComponents> &>(value),
        reinterpret_cast<const vtkTuple<Underlying, NumComponents> &>(nullValue),
        arrayName);
}

template<typename ValueType>
void checkSetArray(vtkFieldData & fieldData, ValueType value, ValueType nullValue, const char * arrayName)
{
    vtkTuple<ValueType, 1> v{ value }, n{ nullValue };
    return checkSetArray(fieldData, v, n, arrayName);
}

void checkSetArray(vtkFieldData & fieldData, const QString & value, const char * arrayName)
{
    int arrayIndex;
    auto abstractArray = fieldData.GetAbstractArray(arrayName, arrayIndex);
    vtkSmartPointer<vtkCharArray> charArray = vtkCharArray::SafeDownCast(abstractArray);
    if (abstractArray && !charArray)    // existing array with invalid type
    {
        fieldData.RemoveArray(arrayIndex);
        abstractArray = nullptr;
        charArray = nullptr;
    }

    if (!value.isEmpty())
    {
        if (!charArray)
        {
            charArray = vtkSmartPointer<vtkCharArray>::New();
            charArray->SetName(arrayName);
            fieldData.AddArray(charArray);
        }
        qstringToVtkArray(value, *charArray);
    }
    else if (charArray)
    {
        fieldData.RemoveArray(arrayIndex);
    }
}

template<typename ValueType, int NumComponents = 1>
void readIfExists(vtkFieldData & fieldData, const char * arrayName, vtkTuple<ValueType, NumComponents> & result)
{
    auto array = fieldData.GetAbstractArray(arrayName);
    if (!array)
    {
        return;
    }
    if (array->GetDataType() != vtkTypeForValueType<ValueType>()
        || array->GetNumberOfComponents() != NumComponents
        || array->GetNumberOfValues() == 0)
    {
        return;
    }
    bool isValid = true;
    std::remove_reference_t<decltype(result)> tmp;
    for (int i = 0; i < NumComponents && isValid; ++i)
    {
        tmp[i] = vtkVariantToValue<ValueType>(array->GetVariantValue(i), &isValid);
    }
    if (isValid)
    {
        result = tmp;
    }
}

template<typename ValueType, typename = std::enable_if_t<std::is_scalar<ValueType>::value>>
void readIfExists(vtkFieldData & fieldData, const char * arrayName, ValueType & result)
{
    vtkTuple<ValueType, 1> r { result };
    readIfExists(fieldData, arrayName, r);
    result = r[0];
}

void readIfExists(vtkFieldData & fieldData, const char * arrayName, QString & result)
{
    if (auto array = fieldData.GetArray(arrayName))
    {
        result = vtkArrayToQString(*array);
    }
    else
    {
        result = QString();
    }
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

bool CoordinateSystemSpecification::operator!=(const CoordinateSystemSpecification & other) const
{
    return !(*this == other);
}

bool CoordinateSystemSpecification::operator==(const ReferencedCoordinateSystemSpecification & referencedSpec) const
{
    return referencedSpec == *this;
}

bool CoordinateSystemSpecification::operator!=(const ReferencedCoordinateSystemSpecification & referencedSpec) const
{
    return !(*this == referencedSpec);
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

void CoordinateSystemSpecification::readFromFieldData(vtkFieldData & fieldData)
{
    *this = CoordinateSystemSpecification();

    readIfExists(fieldData, arrayName_type(), type.value);
    readIfExists(fieldData, arrayName_geographicSystem(), geographicSystem);
    readIfExists(fieldData, arrayName_metricSystem(), globalMetricSystem);
}

void CoordinateSystemSpecification::writeToFieldData(vtkFieldData & fieldData) const
{
    static_assert(std::is_same<std::underlying_type_t<CoordinateSystemType::Value>, int32_t>::value,
        "Unexpected enum type of CoordinateSystemType::Value");

    checkSetArray(fieldData, type.value, CoordinateSystemType::unspecified, arrayName_type());
    checkSetArray(fieldData, geographicSystem, arrayName_geographicSystem());
    checkSetArray(fieldData, globalMetricSystem, arrayName_metricSystem());
}

ReferencedCoordinateSystemSpecification CoordinateSystemSpecification::fromFieldData(vtkFieldData & fieldData)
{
    ReferencedCoordinateSystemSpecification spec;
    spec.readFromFieldData(fieldData);
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

bool ReferencedCoordinateSystemSpecification::operator!=(const CoordinateSystemSpecification & unreferencedSpec) const
{
    return !(*this == unreferencedSpec);
}

bool ReferencedCoordinateSystemSpecification::operator==(const ReferencedCoordinateSystemSpecification & other) const
{
    return CoordinateSystemSpecification::operator==(static_cast<const CoordinateSystemSpecification &>(other))
        && referencePointLatLong == other.referencePointLatLong
        && referencePointLocalRelative == other.referencePointLocalRelative;
}

bool ReferencedCoordinateSystemSpecification::operator!=(const ReferencedCoordinateSystemSpecification & other) const
{
    return !(*this == other);
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

void ReferencedCoordinateSystemSpecification::readFromFieldData(vtkFieldData & fieldData)
{
    CoordinateSystemSpecification::readFromFieldData(fieldData);

    uninitializeVector(referencePointLatLong);
    uninitializeVector(referencePointLocalRelative);

    readIfExists(fieldData, arrayName_geoReference(), referencePointLatLong);
    readIfExists(fieldData, arrayName_relativeReference(), referencePointLocalRelative);
}

void ReferencedCoordinateSystemSpecification::writeToFieldData(vtkFieldData & fieldData) const
{
    CoordinateSystemSpecification::writeToFieldData(fieldData);

    const auto defaultValue = uninitializedVector<double, 2>();

    checkSetArray(fieldData, referencePointLatLong, defaultValue, arrayName_geoReference());
    checkSetArray(fieldData, referencePointLocalRelative, defaultValue, arrayName_relativeReference());
}

ReferencedCoordinateSystemSpecification ReferencedCoordinateSystemSpecification::fromFieldData(vtkFieldData & fieldData)
{
    ReferencedCoordinateSystemSpecification spec;
    spec.readFromFieldData(fieldData);
    return spec;
}

std::ostream & operator<<(std::ostream & os, const CoordinateSystemType & coordsType)
{
    os << coordsType.toString().toStdString();
    return os;
}

std::ostream & operator<<(std::ostream & os, const CoordinateSystemSpecification & spec)
{
    os << spec.type << " (" << spec.geographicSystem.toStdString() << ", " << spec.globalMetricSystem.toStdString() << ")";
    return os;
}

std::ostream & operator<<(std::ostream & os, const ReferencedCoordinateSystemSpecification & spec)
{
    static const char degree = 248;

    os << static_cast<const CoordinateSystemSpecification &>(spec)
        << " (reference point, global: ["
        << spec.referencePointLatLong[0] << degree << " N, "
        << spec.referencePointLatLong[1] << degree << " E], "
        << "relative: ["
        << spec.referencePointLocalRelative[0] << "x, "
        << spec.referencePointLocalRelative[1] << "y])";
    return os;
}
