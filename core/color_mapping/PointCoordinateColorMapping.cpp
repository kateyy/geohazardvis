#include "PointCoordinateColorMapping.h"

#include <vtkMapper.h>
#include <vtkPolyData.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/data_objects/DataObject.h>
#include <core/filters/AssignPointAttributeToCoordinatesFilter.h>
#include <core/utility/DataExtent.h>


namespace
{
const QString s_name = "Point Coordinate";
}

const bool PointCoordinateColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);


std::vector<std::unique_ptr<ColorMappingData>> PointCoordinateColorMapping::newInstances(const std::vector<AbstractVisualizedData*> & visualizedData)
{
    std::vector<AbstractVisualizedData *> polyDataObjects;

    for (auto vis : visualizedData)
    {
        for (int port = 0; port < vis->numberOfColorMappingInputs(); ++port)
        {
            if (vtkPolyData::SafeDownCast(vis->dataObject().dataSet()))
            {
                polyDataObjects.emplace_back(vis);
                break;
            }
        }
    }

    if (polyDataObjects.empty())
    {
        return{};
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    auto mapping = std::make_unique<PointCoordinateColorMapping>(polyDataObjects);
    if (mapping->isValid())
    {
        mapping->initialize();
        instances.push_back(std::move(mapping));
    }

    return instances;
}

PointCoordinateColorMapping::PointCoordinateColorMapping(const std::vector<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData, 3)
{
    setDataComponent(2);    // elevation mapping by default
    m_isValid = true;
}

PointCoordinateColorMapping::~PointCoordinateColorMapping() = default;

QString PointCoordinateColorMapping::name() const
{
    return s_name;
}

QString PointCoordinateColorMapping::scalarsName(AbstractVisualizedData & vis) const
{
    if (auto poly = vtkPolyData::SafeDownCast(vis.dataObject().dataSet()))
    {
        return QString::fromUtf8(poly->GetPoints()->GetData()->GetName());
    }
    return{};
}

IndexType PointCoordinateColorMapping::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return IndexType::points;
}

vtkSmartPointer<vtkAlgorithm> PointCoordinateColorMapping::createFilter(
    AbstractVisualizedData & visualizedData,
    int connection)
{
    auto coordsAttribute = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
    coordsAttribute->CurrentCoordinatesAsScalarsOn();
    coordsAttribute->SetInputConnection(visualizedData.colorMappingInput(connection));

    return coordsAttribute;
}

bool PointCoordinateColorMapping::usesFilter() const
{
    return true;
}

void PointCoordinateColorMapping::configureMapper(
    AbstractVisualizedData & visualizedData,
    vtkAbstractMapper & mapper,
    int connection)
{
    ColorMappingData::configureMapper(visualizedData, mapper, connection);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        if (auto poly = vtkPolyData::SafeDownCast(visualizedData.colorMappingInputData(connection)))
        {
            m->ScalarVisibilityOn();
            m->SetColorModeToMapScalars();
            m->SetScalarModeToUsePointFieldData();
            if (auto points = poly->GetPoints())
            {
                m->SelectColorArray(points->GetData()->GetName());
            }
        }
    }
}

std::vector<ValueRange<>> PointCoordinateColorMapping::updateBounds()
{
    DataBounds dataSetBounds;
    DataBounds totalBounds;

    for (auto vis: m_visualizedData)
    {
        for (int port = 0; port < vis->numberOfColorMappingInputs(); ++port)
        {
            if (auto polyData = vtkPolyData::SafeDownCast(vis->colorMappingInputData(port)))
            {
                polyData->GetBounds(dataSetBounds.data());
                totalBounds.add(dataSetBounds);
            }
        }
    }

    return{
        totalBounds.extractDimension(0),
        totalBounds.extractDimension(1),
        totalBounds.extractDimension(2),
    };
}
