#include "VertexComponentColorMapping.h"

#include <vtkAssignAttribute.h>
#include <vtkMapper.h>

#include <core/types.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/filters/CentroidAsScalarsFilter.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/utility/DataExtent.h>


namespace
{
const QString s_name = "vertex component";
}

const bool VertexComponentColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);


std::vector<std::unique_ptr<ColorMappingData>> VertexComponentColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
{
    QList<AbstractVisualizedData *> polyDataObjects;

    // list all available array names, check for same number of components
    for (AbstractVisualizedData * vis : visualizedData)
    {
        if (dynamic_cast<RenderedPolyData *>(vis))
            polyDataObjects << vis;
    }

    if (polyDataObjects.isEmpty())
        return{};

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (int component = 0; component < 3; ++component)
    {
        auto mapping = std::make_unique<VertexComponentColorMapping>(polyDataObjects, component);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

VertexComponentColorMapping::VertexComponentColorMapping(const QList<AbstractVisualizedData *> & visualizedData, int component)
    : ColorMappingData(visualizedData)
    , m_component{ component }
{
    m_isValid = true;
}

VertexComponentColorMapping::~VertexComponentColorMapping() = default;

QString VertexComponentColorMapping::name() const
{
    return QString('x' + char(m_component)) + "-coordinate";
}

vtkSmartPointer<vtkAlgorithm> VertexComponentColorMapping::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    PolyDataObject * polyData = static_cast<PolyDataObject *>(&visualizedData->dataObject());

    auto centroids = vtkSmartPointer<CentroidAsScalarsFilter>::New();
    centroids->SetInputConnection(0, visualizedData->colorMappingInput(connection));
    centroids->SetInputConnection(1, polyData->cellCentersOutputPort());
    centroids->SetComponent(m_component);

    auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
    assign->SetInputConnection(centroids->GetOutputPort());
    assign->Assign(name().toUtf8().data(), vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::CELL_DATA);

    return assign;
}

bool VertexComponentColorMapping::usesFilter() const
{
    return true;
}

void VertexComponentColorMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);

    if (auto m = vtkMapper::SafeDownCast(mapper))
        m->ScalarVisibilityOn();
}

std::vector<ValueRange<>> VertexComponentColorMapping::updateBounds()
{
    // get min/max coordinate values on our axis/component

    decltype(updateBounds())::value_type totalRange;

    for (auto vis: m_visualizedData)
    {
        DataBounds objectBounds;
        vis->dataObject().bounds(objectBounds.data());

        totalRange.add(objectBounds.extractDimension(m_component));
    }

    return{ totalRange };
}
