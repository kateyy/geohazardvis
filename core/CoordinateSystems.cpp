#include "CoordinateSystems.h"

#include <algorithm>
#include <ostream>

#include <QDataStream>

#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>

#include <core/filters/vtkInformationDoubleVectorMetaDataKey.h>
#include <core/filters/vtkInformationIntegerMetaDataKey.h>
#include <core/filters/vtkInformationStringMetaDataKey.h>
#include <core/utility/vtkstringhelper.h>
#include <core/utility/vtkvarianthelper.h>
#include <core/utility/vtkvectorhelper.h>


vtkInformationKeyMacro(CoordinateSystemSpecification, CoordinateSystemType_InfoKey, IntegerMetaData);
vtkInformationKeyMacro(CoordinateSystemSpecification, GeographicCoordinateSystemName_InfoKey, StringMetaData);
vtkInformationKeyMacro(CoordinateSystemSpecification, MetricCoordinateSystemName_InfoKey, StringMetaData);
vtkInformationKeyMacro(CoordinateSystemSpecification, UnitOfMeasurement_InfoKey, StringMetaData);
vtkInformationKeyMacro(ReferencedCoordinateSystemSpecification, ReferencePointLatLong_InfoKey, DoubleVectorMetaData);


namespace
{

bool streamOperatorsRegistered = [] ()
{
    qRegisterMetaTypeStreamOperators<CoordinateSystemType>(
        "CoordinateSystemType");
    qRegisterMetaTypeStreamOperators<CoordinateSystemSpecification>(
        "CoordinateSystemSpecification");
    qRegisterMetaTypeStreamOperators<ReferencedCoordinateSystemSpecification>(
        "ReferencedCoordinateSystemSpecification");

    return true;
}();

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

const char * arrayName_unitOfMeasurement()
{
    static const char * const name = "CoordinateSystem_UnitOfMeasurement";
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
    vtkSmartPointer<vtkCharArray> charArray = vtkCharArray::FastDownCast(abstractArray);
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
    if (auto array = fieldData.GetAbstractArray(arrayName))
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
    , unitOfMeasurement{}
{
}

CoordinateSystemSpecification::CoordinateSystemSpecification(
    CoordinateSystemType type,
    const QString & geographicSystem,
    const QString & globalMetricSystem,
    const QString & unitOfMeasurement)
    : type{ type }
    , geographicSystem{ geographicSystem }
    , globalMetricSystem{ globalMetricSystem }
    , unitOfMeasurement{ unitOfMeasurement }
{
}

bool CoordinateSystemSpecification::isValid() const
{
    if (type == CoordinateSystemType::unspecified)
    {
        return false;
    }

    if (type == CoordinateSystemType::geographic
        || type == CoordinateSystemType::other)
    {
        return !geographicSystem.isEmpty();
    }

    return !geographicSystem.isEmpty() && !globalMetricSystem.isEmpty() && !unitOfMeasurement.isEmpty();
}

bool CoordinateSystemSpecification::isUnspecified() const
{
    return type == CoordinateSystemType::unspecified
        && geographicSystem.isEmpty() && globalMetricSystem.isEmpty() && unitOfMeasurement.isEmpty();
}

bool CoordinateSystemSpecification::operator==(const CoordinateSystemSpecification & other) const
{
    return type == other.type
        && geographicSystem == other.geographicSystem
        && globalMetricSystem == other.globalMetricSystem
        && unitOfMeasurement == other.unitOfMeasurement;
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
    if (info.Has(UnitOfMeasurement_InfoKey()))
    {
        unitOfMeasurement = QString::fromUtf8(info.Get(UnitOfMeasurement_InfoKey()));
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
    if (!unitOfMeasurement.isEmpty())
    {
        info.Set(UnitOfMeasurement_InfoKey(), unitOfMeasurement.toUtf8().data());
    }
    else
    {
        info.Remove(UnitOfMeasurement_InfoKey());
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
    readIfExists(fieldData, arrayName_unitOfMeasurement(), unitOfMeasurement);
}

void CoordinateSystemSpecification::writeToFieldData(vtkFieldData & fieldData) const
{
    static_assert(std::is_same<std::underlying_type_t<CoordinateSystemType::Value>, int32_t>::value,
        "Unexpected enum type of CoordinateSystemType::Value");

    checkSetArray(fieldData, type.value, CoordinateSystemType::unspecified, arrayName_type());
    checkSetArray(fieldData, geographicSystem, arrayName_geographicSystem());
    checkSetArray(fieldData, globalMetricSystem, arrayName_metricSystem());
    checkSetArray(fieldData, unitOfMeasurement, arrayName_unitOfMeasurement());
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
}

ReferencedCoordinateSystemSpecification::ReferencedCoordinateSystemSpecification(
    CoordinateSystemType type,
    const QString & geographicSystem,
    const QString & globalMetricSystem,
    const QString & unitOfMeasurement,
    vtkVector2d referencePointLatLong)
    : CoordinateSystemSpecification(type, geographicSystem, globalMetricSystem, unitOfMeasurement)
    , referencePointLatLong{ referencePointLatLong }
{
}

ReferencedCoordinateSystemSpecification::ReferencedCoordinateSystemSpecification(
    CoordinateSystemSpecification unreferencedSpec,
    vtkVector2d referencePointLatLong)
    : CoordinateSystemSpecification(unreferencedSpec)
    , referencePointLatLong{ referencePointLatLong }
{
}

bool ReferencedCoordinateSystemSpecification::isReferencePointValid() const
{
    return isVectorInitialized(referencePointLatLong);
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
    if (!CoordinateSystemSpecification::operator==(static_cast<const CoordinateSystemSpecification &>(other)))
    {
        return false;
    }

    return (referencePointLatLong == other.referencePointLatLong)
        || (!isVectorInitialized(referencePointLatLong)
            && !isVectorInitialized(other.referencePointLatLong));
}

bool ReferencedCoordinateSystemSpecification::operator!=(const ReferencedCoordinateSystemSpecification & other) const
{
    return !(*this == other);
}

void ReferencedCoordinateSystemSpecification::readFromInformation(vtkInformation & info)
{
    CoordinateSystemSpecification::readFromInformation(info);

    uninitializeVector(referencePointLatLong);

    if (info.Has(ReferencePointLatLong_InfoKey()))
    {
        referencePointLatLong = vtkVector2d(info.Get(ReferencePointLatLong_InfoKey()));
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

    readIfExists(fieldData, arrayName_geoReference(), referencePointLatLong);
}

void ReferencedCoordinateSystemSpecification::writeToFieldData(vtkFieldData & fieldData) const
{
    CoordinateSystemSpecification::writeToFieldData(fieldData);

    const auto defaultValue = uninitializedVector<double, 2>();

    checkSetArray(fieldData, referencePointLatLong, defaultValue, arrayName_geoReference());
}

ReferencedCoordinateSystemSpecification ReferencedCoordinateSystemSpecification::fromFieldData(vtkFieldData & fieldData)
{
    ReferencedCoordinateSystemSpecification spec;
    spec.readFromFieldData(fieldData);
    return spec;
}

QDataStream & operator<<(QDataStream & stream, const CoordinateSystemType & coordsType)
{
    stream << coordsType.toString();
    return stream;
}

QDataStream & operator>>(QDataStream & stream, CoordinateSystemType & coordsType)
{
    QString type;
    stream >> type;
    coordsType.fromString(type);
    return stream;
}

std::ostream & operator<<(std::ostream & os, const CoordinateSystemType & coordsType)
{
    os << coordsType.toString().toStdString();
    return os;
}

QDataStream & operator<<(QDataStream & stream, const CoordinateSystemSpecification & spec)
{
    stream << spec.type
        << spec.geographicSystem
        << spec.globalMetricSystem
        << spec.unitOfMeasurement;
    return stream;
}

QDataStream & operator>>(QDataStream & stream, CoordinateSystemSpecification & spec)
{
    stream >> spec.type
        >> spec.geographicSystem
        >> spec.globalMetricSystem
        >> spec.unitOfMeasurement;
    return stream;
}

std::ostream & operator<<(std::ostream & os, const CoordinateSystemSpecification & spec)
{
    os << spec.type << " (" << spec.geographicSystem.toStdString() << ", " << spec.globalMetricSystem.toStdString() << ")";
    return os;
}

QDataStream & operator<<(QDataStream & stream, const ReferencedCoordinateSystemSpecification & spec)
{
    stream << static_cast<const CoordinateSystemSpecification &>(spec)
        << spec.referencePointLatLong.GetX() << spec.referencePointLatLong.GetY();
    return stream;
}

QDataStream & operator>>(QDataStream & stream, ReferencedCoordinateSystemSpecification & spec)
{
    double refX = {}, refY = {};
    stream >> static_cast<CoordinateSystemSpecification &>(spec)
        >> refX >> refY;
    spec.referencePointLatLong = { refX, refY };
    return stream;
}

std::ostream & operator<<(std::ostream & os, const ReferencedCoordinateSystemSpecification & spec)
{
    static const auto degree = static_cast<unsigned char>(248);

    os << static_cast<const CoordinateSystemSpecification &>(spec)
        << " (reference point, global: ["
        << spec.referencePointLatLong[0] << degree << " N, "
        << spec.referencePointLatLong[1] << degree << " E], ";
    return os;
}
