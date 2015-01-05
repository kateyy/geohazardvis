#include "PolyDataAttributeVectorMapping.h"

#include <cstring>

#include <vtkPolyData.h>

#include <vtkCellData.h>

#include <vtkGlyph3D.h>

#include <vtkAssignAttribute.h>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/vector_mapping/VectorMappingRegistry.h>


namespace
{
const QString s_name = "poly data attribute vectors";
}

const bool PolyDataAttributeVectorMapping::s_registered = VectorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

using namespace reflectionzeug;


QList<VectorMappingData *> PolyDataAttributeVectorMapping::newInstances(RenderedData * renderedData)
{
    vtkPolyData * polyData = vtkPolyData::SafeDownCast(renderedData->dataObject()->processedDataSet());
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
        if (name == "centroid")
            continue;

        if (a->GetInformation()->Has(DataObject::ArrayIsAuxiliaryKey())
            && a->GetInformation()->Get(DataObject::ArrayIsAuxiliaryKey()))
            continue;

        vectorArrays << a;
    }

    QList<VectorMappingData *> instances;
    for (vtkDataArray * vectorsArray : vectorArrays)
    {
        PolyDataAttributeVectorMapping * mapping = new PolyDataAttributeVectorMapping(renderedData, vectorsArray);
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

PolyDataAttributeVectorMapping::PolyDataAttributeVectorMapping(RenderedData * renderedData, vtkDataArray * vectorData)
    : VectorMappingData(renderedData)
    , m_polyData(dynamic_cast<PolyDataObject *>(renderedData->dataObject()))
    , m_dataArray(vectorData)
{
    m_isValid = m_isValid && m_polyData != nullptr;

    arrowGlyph()->SetVectorModeToUseVector();
}

PolyDataAttributeVectorMapping::~PolyDataAttributeVectorMapping() = default;

QString PolyDataAttributeVectorMapping::name() const
{
    assert(m_dataArray);
    return QString::fromUtf8(m_dataArray->GetName());
}

void PolyDataAttributeVectorMapping::initialize()
{
    VTK_CREATE(vtkAssignAttribute, assignAttribute);
    assignAttribute->SetInputConnection(m_polyData->cellCentersOutputPort());
    assignAttribute->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    arrowGlyph()->SetInputConnection(assignAttribute->GetOutputPort());
}
