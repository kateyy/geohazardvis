#include "VertexComponentMapping.h"

#include <vtkDataSet.h>
#include <vtkAssignAttribute.h>
#include <vtkMapper.h>

#include <core/vtkhelper.h>
#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/filters/CentroidAsScalarsFilter.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


namespace
{
const QString s_name = "vertex component";
}

const bool VertexComponentMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);


QList<ScalarsForColorMapping *> VertexComponentMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
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

    QList<ScalarsForColorMapping *> instances;
    for (vtkIdType component = 0; component < 3; ++component)
    {
        VertexComponentMapping * mapping = new VertexComponentMapping(polyDataObjects, component);
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

VertexComponentMapping::VertexComponentMapping(const QList<AbstractVisualizedData *> & visualizedData, vtkIdType component)
    : ScalarsForColorMapping(visualizedData)
    , m_component(component)
{
    m_isValid = true;
}

VertexComponentMapping::~VertexComponentMapping() = default;

QString VertexComponentMapping::name() const
{
    return QString('x' + char(m_component)) + "-coordinate";
}

vtkAlgorithm * VertexComponentMapping::createFilter(AbstractVisualizedData * visualizedData)
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

bool VertexComponentMapping::usesFilter() const
{
    return true;
}

void VertexComponentMapping::configureMapper(AbstractVisualizedData * /*visualizedData*/, vtkMapper * mapper)
{
    mapper->ScalarVisibilityOn();
}

void VertexComponentMapping::updateBounds()
{
    // get min/max coordinate values on our axis/component

    double totalRange[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };
    for (AbstractVisualizedData * vis: m_visualizedData)
    {
        const double * objectBounds = vis->dataObject()->dataSet()->GetBounds();

        totalRange[0] = std::min(totalRange[0], objectBounds[2 * m_component]);
        totalRange[1] = std::max(totalRange[1], objectBounds[2 * m_component + 1]);
    }

    setDataMinMaxValue(totalRange, 0);
}
