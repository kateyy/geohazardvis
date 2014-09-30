#include "VertexComponentMapping.h"

#include <vtkDataSet.h>
#include <vtkAssignAttribute.h>

#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/filters/CentroidAsScalarsFilter.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


namespace
{
const QString s_name = "vertex component";
}

const bool VertexComponentMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);


QList<ScalarsForColorMapping *> VertexComponentMapping::newInstances(const QList<DataObject *> & dataObjects)
{
    QList<DataObject *> polyDataObjects;

    // list all available array names, check for same number of components
    for (DataObject * dataObject : dataObjects)
    {
        if ( dynamic_cast<PolyDataObject *>(dataObject))
            polyDataObjects << dataObject;
    }

    QList<ScalarsForColorMapping *> instances;
    for (auto polyDataObject : polyDataObjects)
    {
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
    }

    return instances;
}

VertexComponentMapping::VertexComponentMapping(const QList<DataObject *> & dataObjects, vtkIdType component)
    : ScalarsForColorMapping(dataObjects)
    , m_component(component)
{
}

VertexComponentMapping::~VertexComponentMapping() = default;

QString VertexComponentMapping::name() const
{
    return QString('x' + char(m_component)) + "-coordinate";
}

vtkAlgorithm * VertexComponentMapping::createFilter(DataObject * dataObject)
{
    PolyDataObject * polyData = static_cast<PolyDataObject *>(dataObject);

    VTK_CREATE(CentroidAsScalarsFilter, centroids);
    centroids->SetInputConnection(0, polyData->processedOutputPort());
    centroids->SetInputConnection(1, polyData->cellCentersOutputPort());
    centroids->SetComponent(m_component);

    vtkAssignAttribute * assign = vtkAssignAttribute::New();
    assign->SetInputConnection(centroids->GetOutputPort());
    assign->Assign(name().toLatin1().data(), vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::CELL_DATA);

    return assign;
}

bool VertexComponentMapping::usesFilter() const
{
    return true;
}

bool VertexComponentMapping::isValid() const
{
    return true;
}

void VertexComponentMapping::updateBounds()
{
    QByteArray c_name = name().toLatin1();

    // get min/max coordinate values on our axis/component

    double totalRange[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };
    for (DataObject * dataObject : m_dataObjects)
    {
        const double * objectBounds = dataObject->dataSet()->GetBounds();

        totalRange[0] = std::min(totalRange[0], objectBounds[2 * m_component]);
        totalRange[1] = std::max(totalRange[1], objectBounds[2 * m_component + 1]);
    }

    setDataMinMaxValue(totalRange);
}
