#include "SlopeAngleMapping.h"

#include <cassert>
#include <sstream>

#include <vtkAssignAttribute.h>
#include <vtkArrayCalculator.h>
#include <vtkMath.h>
#include <vtkMapper.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/utility/type_traits.h>
#include <core/utility/DataExtent.h>


namespace
{
const QString l_name = "slope angle mapping";
const QString l_dataArrayName = "Slope Angle";
}

const bool SlopeAngleMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    l_name,
    &SlopeAngleMapping::newInstance);

std::vector<std::unique_ptr<ColorMappingData>> SlopeAngleMapping::newInstance(const QList<AbstractVisualizedData *> & visualizedData)
{
    Unqualified<decltype(visualizedData)> validVisualizations;

    for (auto vis : visualizedData)
    {
        if (auto polyData = dynamic_cast<PolyDataObject *>(&vis->dataObject()))
        {
            if (polyData->is2p5D())
            {
                validVisualizations << vis;
            }
        }
    }

    decltype(newInstance(visualizedData)) result;
    if (!validVisualizations.empty())
    {
        result.push_back(std::make_unique<SlopeAngleMapping>(validVisualizations));
    }

    return result;
}

SlopeAngleMapping::SlopeAngleMapping(const QList<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
{
    assert(!visualizedData.isEmpty());

    m_isValid = true;
}

SlopeAngleMapping::~SlopeAngleMapping() = default;

QString SlopeAngleMapping::name() const
{
    return l_dataArrayName;
}

QString SlopeAngleMapping::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return l_dataArrayName;
}

IndexType SlopeAngleMapping::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return IndexType::cells;
}

vtkSmartPointer<vtkAlgorithm> SlopeAngleMapping::createFilter(AbstractVisualizedData & visualizedData, int connection)
{
    static const auto arrayName = scalarsName(visualizedData).toUtf8();

    if (m_filters.contains(&visualizedData))
    {
        if (m_filters[&visualizedData].contains(connection))
        {
            return m_filters[&visualizedData][connection];
        }
    }

    auto calculator = vtkSmartPointer<vtkArrayCalculator>::New();
    calculator->SetInputConnection(visualizedData.colorMappingInput(connection));
    calculator->AddScalarVariable("z", "Normals", 2);
    calculator->SetResultArrayName(arrayName.data());
    calculator->SetAttributeModeToUseCellData();
    static const auto fun = "acos(z) * 180 / " + [] () {
        std::stringstream piStr;
        piStr.precision(16);
        piStr << vtkMath::Pi(); 
        return piStr.str();
    }();
    calculator->SetFunction(fun.c_str());

    auto assignAttibute = vtkSmartPointer<vtkAssignAttribute>::New();
    assignAttibute->SetInputConnection(calculator->GetOutputPort());
    assignAttibute->Assign(arrayName.data(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);

    auto unitSetter = vtkSmartPointer<ArrayChangeInformationFilter>::New();
    unitSetter->SetInputConnection(assignAttibute->GetOutputPort());
    unitSetter->SetAttributeLocation(ArrayChangeInformationFilter::CELL_DATA);
    unitSetter->SetAttributeType(vtkDataSetAttributes::SCALARS);
    unitSetter->EnableRenameOff();
    unitSetter->EnableSetUnitOn();
    unitSetter->SetArrayUnit(QString(QChar(0x00B0)).toUtf8().data());   // degree sign

    m_filters[&visualizedData][connection] = unitSetter;

    return unitSetter;
}

bool SlopeAngleMapping::usesFilter() const
{
    return true;
}

void SlopeAngleMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & abstractMapper)
{
    ColorMappingData::configureMapper(visualizedData, abstractMapper);

    auto mapper = vtkMapper::SafeDownCast(&abstractMapper);
    if (!mapper)
    {
        return;
    }

    mapper->ScalarVisibilityOn();
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SelectColorArray(scalarsName(visualizedData).toUtf8().data());
}

std::vector<ValueRange<>> SlopeAngleMapping::updateBounds()
{
    return{ decltype(updateBounds())::value_type({ 0.0, 90.0 }) };
}
