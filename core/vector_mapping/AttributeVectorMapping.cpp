#include "AttributeVectorMapping.h"

#include <cstring>

#include <vtkPolyData.h>

#include <vtkCellData.h>

#include <vtkGlyph3D.h>

#include <vtkAssignAttribute.h>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "cell data vectors";
}

const bool AttributeVectorMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

using namespace reflectionzeug;


QList<VectorsForSurfaceMapping *> AttributeVectorMapping::newInstances(RenderedData * renderedData)
{
    vtkPolyData * polyData = vtkPolyData::SafeDownCast(renderedData->dataObject()->dataSet());
    // only polygonal datasets are supported
    if (!polyData)
        return{};


    // find 3D-vector data, skip the "centroid"

    vtkCellData * cellData = polyData->GetCellData();
    QList<vtkDataArray *> vectorArrays;
    for (int i = 0; vtkDataArray * a = cellData->GetArray(i); ++i)
    {
        assert(a);
        QString name(a->GetName());
        if (name == "centroid" || name == "Normals")
            continue;

        if (a->GetInformation()->Has(DataObject::ArrayIsAuxiliaryKey())
            && a->GetInformation()->Get(DataObject::ArrayIsAuxiliaryKey()))
            continue;

        vectorArrays << a;
    }

    QList<VectorsForSurfaceMapping *> instances;
    for (vtkDataArray * vectorsArray : vectorArrays)
    {
        AttributeVectorMapping * mapping = new AttributeVectorMapping(renderedData, vectorsArray);
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

AttributeVectorMapping::AttributeVectorMapping(RenderedData * renderedData, vtkDataArray * vectorData)
    : VectorsForSurfaceMapping(renderedData)
    , m_dataArray(vectorData)
    , m_polyData(dynamic_cast<PolyDataObject *>(renderedData->dataObject()))
{
    if (!m_isValid || !m_polyData)
        return;

    arrowGlyph()->SetVectorModeToUseVector();
}

AttributeVectorMapping::~AttributeVectorMapping() = default;

QString AttributeVectorMapping::name() const
{
    assert(m_dataArray);
    return QString::fromLatin1(m_dataArray->GetName());
}

void AttributeVectorMapping::initialize()
{
    VTK_CREATE(vtkAssignAttribute, assignAttribute);
    assignAttribute->SetInputConnection(m_polyData->cellCentersOutputPort());
    assignAttribute->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    arrowGlyph()->SetInputConnection(assignAttribute->GetOutputPort());
}
