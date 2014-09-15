#include "AttributeVectorMapping.h"

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkPoints.h>

#include <vtkPointData.h>
#include <vtkCellData.h>

#include <vtkGlyph3D.h>
#include <vtkVertexGlyphFilter.h>

#include <core/DataSetHandler.h>
#include <core/vtkhelper.h>
#include <core/data_objects/AttributeVectorData.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "attribute array vectors";
}

const bool AttributeVectorMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<VectorsForSurfaceMapping *> AttributeVectorMapping::newInstances(RenderedData * renderedData)
{
    QList<AttributeVectorData *> attrs;

    for (DataObject * dataObject : DataSetHandler::instance().dataObjects())
    {
        AttributeVectorData * attr = dynamic_cast<AttributeVectorData *>(dataObject);
        if (!attr)
            continue;

        if (attr->dataArray()->GetNumberOfTuples() >= renderedData->dataObject()->dataSet()->GetNumberOfCells())
            attrs << attr;
    }

    QList<VectorsForSurfaceMapping *> instances;
    for (AttributeVectorData * attr : attrs)
    {
        AttributeVectorMapping * mapping = new AttributeVectorMapping(renderedData, attr);
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

AttributeVectorMapping::AttributeVectorMapping(RenderedData * renderedData, AttributeVectorData * attributeVector)
    : VectorsForSurfaceMapping(renderedData)
    , m_attributeVector(attributeVector)
{
    if (!m_isValid)
        return;

    arrowGlyph()->SetVectorModeToUseVector();
}

AttributeVectorMapping::~AttributeVectorMapping() = default;

QString AttributeVectorMapping::name() const
{
    assert(m_attributeVector);
    return QString::fromLatin1(m_attributeVector->dataArray()->GetName());
}

void AttributeVectorMapping::initialize()
{
    vtkCellData * cellData = polyData()->GetCellData();

    vtkSmartPointer<vtkDataArray> centroids = cellData->GetArray("centroid");
    assert(centroids);

    VTK_CREATE(vtkPoints, points);
    points->SetData(centroids);

    VTK_CREATE(vtkPolyData, pointsPolyData);
    pointsPolyData->SetPoints(points);

    VTK_CREATE(vtkVertexGlyphFilter, filter);
    filter->SetInputData(pointsPolyData);
    filter->Update();
    vtkPolyData * processedPoints = filter->GetOutput();

    processedPoints->GetPointData()->SetVectors(m_attributeVector->dataArray());

    arrowGlyph()->SetInputData(processedPoints);
}
