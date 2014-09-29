#include "CellDataVectorMapping.h"

#include <cstring>

#include <vtkPolyData.h>

#include <vtkPointData.h>
#include <vtkCellData.h>

#include <vtkGlyph3D.h>

#include <vtkCellCenters.h>
#include <vtkAssignAttribute.h>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "cell data vectors";
}

const bool CellDataVectorMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

using namespace reflectionzeug;


QList<VectorsForSurfaceMapping *> CellDataVectorMapping::newInstances(RenderedData * renderedData)
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
        CellDataVectorMapping * mapping = new CellDataVectorMapping(renderedData, vectorsArray);
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

CellDataVectorMapping::CellDataVectorMapping(RenderedData * renderedData, vtkDataArray * vectorData)
    : VectorsForSurfaceMapping(renderedData)
    , m_dataArray(vectorData)
{
    if (!m_isValid)
        return;

    arrowGlyph()->SetVectorModeToUseVector();
}

CellDataVectorMapping::~CellDataVectorMapping() = default;

QString CellDataVectorMapping::name() const
{
    assert(m_dataArray);
    return QString::fromLatin1(m_dataArray->GetName());
}

void CellDataVectorMapping::initialize()
{
    assert(polyData());

    VTK_CREATE(vtkCellCenters, centroidPoints);
    centroidPoints->SetInputData(polyData());

    VTK_CREATE(vtkAssignAttribute, assignAttribute);
    assignAttribute->SetInputConnection(centroidPoints->GetOutputPort());
    assignAttribute->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    arrowGlyph()->SetInputConnection(assignAttribute->GetOutputPort());
}
