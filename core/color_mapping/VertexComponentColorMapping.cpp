#include "VertexComponentColorMapping.h"

#include <vtkDataSet.h>
#include <vtkAssignAttribute.h>
#include <vtkMapper.h>

#include <core/vtkhelper.h>
#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/filters/CentroidAsScalarsFilter.h>
#include <core/color_mapping/ColorMappingRegistry.h>


namespace
{
const QString s_name = "vertex component";
}

const bool VertexComponentColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);


QList<ColorMappingData *> VertexComponentColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
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

    QList<ColorMappingData *> instances;
    for (vtkIdType component = 0; component < 3; ++component)
    {
        VertexComponentColorMapping * mapping = new VertexComponentColorMapping(polyDataObjects, component);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances << mapping;
        }
        else
            delete mapping;
    }

    return instances;
}

VertexComponentColorMapping::VertexComponentColorMapping(const QList<AbstractVisualizedData *> & visualizedData, vtkIdType component)
    : ColorMappingData(visualizedData)
    , m_component(component)
{
    m_isValid = true;
}

VertexComponentColorMapping::~VertexComponentColorMapping() = default;

QString VertexComponentColorMapping::name() const
{
    return QString('x' + char(m_component)) + "-coordinate";
}

vtkAlgorithm * VertexComponentColorMapping::createFilter(AbstractVisualizedData * visualizedData)
{
    PolyDataObject * polyData = static_cast<PolyDataObject *>(visualizedData->dataObject());

    VTK_CREATE(CentroidAsScalarsFilter, centroids);
    centroids->SetInputConnection(0, polyData->processedOutputPort());
    centroids->SetInputConnection(1, polyData->cellCentersOutputPort());
    centroids->SetComponent(m_component);

    vtkAssignAttribute * assign = vtkAssignAttribute::New();
    assign->SetInputConnection(centroids->GetOutputPort());
    assign->Assign(name().toUtf8().data(), vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::CELL_DATA);

    return assign;
}

bool VertexComponentColorMapping::usesFilter() const
{
    return true;
}

void VertexComponentColorMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);

    mapper->ScalarVisibilityOn();
}

QMap<vtkIdType, QPair<double, double>> VertexComponentColorMapping::updateBounds()
{
    // get min/max coordinate values on our axis/component

    double totalMin = std::numeric_limits<double>::max();
    double totalMax = std::numeric_limits<double>::lowest();

    for (AbstractVisualizedData * vis: m_visualizedData)
    {
        const double * objectBounds = vis->dataObject()->dataSet()->GetBounds();

        totalMin = std::min(totalMin, objectBounds[2 * m_component]);
        totalMax = std::max(totalMax, objectBounds[2 * m_component + 1]);
    }

    return{ { 0, { totalMin, totalMax } } };
}
