#include "CentroidColorMapping.h"

#include <algorithm>
#include <cassert>

#include <vtkDataSetAttributes.h>
#include <vtkMapper.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/filters/CentroidAsScalarsFilter.h>
#include <core/utility/DataExtent.h>


namespace
{
const QString s_name = "Centroid Component";
const char * s_centroidsArrayName = "Centroid";
}

const bool CentroidColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);


std::vector<std::unique_ptr<ColorMappingData>> CentroidColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
{
    QList<AbstractVisualizedData *> polyDataObjects;

    // list all available array names, check for same number of components
    for (auto vis : visualizedData)
    {
        if (dynamic_cast<PolyDataObject *>(&vis->dataObject()))
        {
            polyDataObjects << vis;
        }
    }

    if (polyDataObjects.isEmpty())
    {
        return{};
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    auto mapping = std::make_unique<CentroidColorMapping>(polyDataObjects);
    if (mapping->isValid())
    {
        mapping->initialize();
        instances.push_back(std::move(mapping));
    }

    return instances;
}

CentroidColorMapping::CentroidColorMapping(const QList<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData, 3)
{
    setDataComponent(2);    // elevation mapping by default
    m_isValid = true;
}

CentroidColorMapping::~CentroidColorMapping() = default;

QString CentroidColorMapping::name() const
{
    return s_name;
}

QString CentroidColorMapping::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return s_centroidsArrayName;
}

IndexType CentroidColorMapping::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return IndexType::cells;
}

vtkSmartPointer<vtkAlgorithm> CentroidColorMapping::createFilter(
    AbstractVisualizedData & visualizedData,
    int connection)
{
    assert(connection >= 0);
    const size_t conn = static_cast<size_t>(std::max(0, connection));

    auto ports = m_filters[&visualizedData];
    if (ports.size() > conn)
    {
        if (auto filter = ports[conn])
        {
            return filter;
        }
    }

    auto polyData = static_cast<PolyDataObject *>(&visualizedData.dataObject());

    auto centroids = vtkSmartPointer<CentroidAsScalarsFilter>::New();
    centroids->SetInputConnection(0, visualizedData.colorMappingInput(connection));
    centroids->SetInputConnection(1, polyData->cellCentersOutputPort());

    auto rename = vtkSmartPointer<ArrayChangeInformationFilter>::New();
    rename->EnableRenameOn();
    rename->SetArrayName(s_centroidsArrayName);
    rename->SetAttributeLocation(ArrayChangeInformationFilter::CELL_DATA);
    rename->SetAttributeType(vtkDataSetAttributes::SCALARS);
    rename->SetInputConnection(centroids->GetOutputPort());

    ports.resize(conn + 1);
    ports[conn] = rename;

    return rename;
}

bool CentroidColorMapping::usesFilter() const
{
    return true;
}

void CentroidColorMapping::configureMapper(
    AbstractVisualizedData & visualizedData,
    vtkAbstractMapper & mapper,
    int connection)
{
    ColorMappingData::configureMapper(visualizedData, mapper, connection);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->ScalarVisibilityOn();
        m->SetColorModeToMapScalars();
        m->SetScalarModeToUseCellFieldData();
        m->SelectColorArray(s_centroidsArrayName);
    }
}

std::vector<ValueRange<>> CentroidColorMapping::updateBounds()
{
    DataBounds dataSetBounds;
    DataBounds totalBounds;

    for (auto vis: m_visualizedData)
    {
        if (auto cellCenters = static_cast<PolyDataObject &>(vis->dataObject()).cellCenters())
        {
            cellCenters->GetBounds(dataSetBounds.data());
            totalBounds.add(dataSetBounds);
        }
    }

    return{
        totalBounds.extractDimension(0),
        totalBounds.extractDimension(1),
        totalBounds.extractDimension(2),
    };
}
