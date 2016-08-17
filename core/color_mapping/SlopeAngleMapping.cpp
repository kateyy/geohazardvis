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
    QList<AbstractVisualizedData *> validVisualizations;

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

QString SlopeAngleMapping::name() const
{
    return l_dataArrayName;
}

QString SlopeAngleMapping::scalarsName() const
{
    return l_dataArrayName;
}

vtkSmartPointer<vtkAlgorithm> SlopeAngleMapping::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    static const auto arrayName = scalarsName().toUtf8();

    if (m_filters.contains(visualizedData))
    {
        if (m_filters[visualizedData].contains(connection))
        {
            return m_filters[visualizedData][connection];
        }
    }

    auto calculator = vtkSmartPointer<vtkArrayCalculator>::New();
    calculator->SetInputConnection(visualizedData->colorMappingInput(connection));
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

    m_filters[visualizedData][connection] = unitSetter;

    return unitSetter;
}

bool SlopeAngleMapping::usesFilter() const
{
    return true;
}

void SlopeAngleMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * abstractMapper)
{
    ColorMappingData::configureMapper(visualizedData, abstractMapper);

    auto mapper = vtkMapper::SafeDownCast(abstractMapper);
    if (!mapper)
    {
        return;
    }

    mapper->ScalarVisibilityOn();
    mapper->SetScalarModeToUseCellData();
    mapper->SelectColorArray(scalarsName().toUtf8().data());
}

QMap<int, QPair<double, double>> SlopeAngleMapping::updateBounds()
{
    return{ { 0, { 0.0, 90.0 } } };
}
