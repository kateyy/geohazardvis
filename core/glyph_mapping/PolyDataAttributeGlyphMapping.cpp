#include "PolyDataAttributeGlyphMapping.h"

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
#include <core/glyph_mapping/GlyphMappingRegistry.h>


namespace
{
const QString s_name = "poly data attribute vectors";
}

const bool PolyDataAttributeGlyphMapping::s_registered = GlyphMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

using namespace reflectionzeug;


QList<GlyphMappingData *> PolyDataAttributeGlyphMapping::newInstances(RenderedData * renderedData)
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

    QList<GlyphMappingData *> instances;
    for (vtkDataArray * vectorsArray : vectorArrays)
    {
        PolyDataAttributeGlyphMapping * mapping = new PolyDataAttributeGlyphMapping(renderedData, vectorsArray);
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

PolyDataAttributeGlyphMapping::PolyDataAttributeGlyphMapping(RenderedData * renderedData, vtkDataArray * vectorData)
    : GlyphMappingData(renderedData)
    , m_polyData(dynamic_cast<PolyDataObject *>(renderedData->dataObject()))
    , m_dataArray(vectorData)
{
    m_isValid = m_isValid && m_polyData != nullptr;

    arrowGlyph()->SetVectorModeToUseVector();

    m_assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignVectors->SetInputConnection(m_polyData->cellCentersOutputPort());
    m_assignVectors->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
}

PolyDataAttributeGlyphMapping::~PolyDataAttributeGlyphMapping() = default;

QString PolyDataAttributeGlyphMapping::name() const
{
    assert(m_dataArray);
    return QString::fromUtf8(m_dataArray->GetName());
}

vtkAlgorithmOutput * PolyDataAttributeGlyphMapping::vectorDataOutputPort()
{
    return m_assignVectors->GetOutputPort();
}
