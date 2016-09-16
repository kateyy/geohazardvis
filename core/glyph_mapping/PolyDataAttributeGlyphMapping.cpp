#include "PolyDataAttributeGlyphMapping.h"

#include <cassert>
#include <cstring>

#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>
#include <vtkPolyData.h>
#include <vtkGlyph3D.h>

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


std::vector<std::unique_ptr<GlyphMappingData>> PolyDataAttributeGlyphMapping::newInstances(RenderedData & renderedData)
{
    auto polyData = dynamic_cast<PolyDataObject *>(&renderedData.dataObject());

    // only polygonal datasets are supported
    if (!polyData)
    {
        return{};
    }

    // find 3D-vector data, skip the "centroid"

    auto cellData = polyData->dataSet()->GetCellData();
    QList<vtkDataArray *> vectorArrays;
    for (int i = 0; i < cellData->GetNumberOfArrays(); ++i)
    {
        auto a = cellData->GetArray(i);

        if (!a || a->GetNumberOfComponents() != 3)
        {
            continue;
        }

        if (QString::fromUtf8(a->GetName()) == "centroid")
        {
            continue;
        }

        if (a->GetInformation()->Has(DataObject::ArrayIsAuxiliaryKey())
            && a->GetInformation()->Get(DataObject::ArrayIsAuxiliaryKey()))
        {
            continue;
        }

        vectorArrays << a;
    }

    std::vector<std::unique_ptr<GlyphMappingData>> instances;
    for (auto vectorsArray : vectorArrays)
    {
        auto mapping = std::make_unique<PolyDataAttributeGlyphMapping>(renderedData, *polyData, *vectorsArray);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

PolyDataAttributeGlyphMapping::PolyDataAttributeGlyphMapping(
    RenderedData & renderedData,
    PolyDataObject & polyDataObject,
    vtkDataArray & vectorData)
    : GlyphMappingData(renderedData)
    , m_dataArray(&vectorData)
{
    arrowGlyph()->SetVectorModeToUseVector();

    m_assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignVectors->SetInputConnection(polyDataObject.cellCentersOutputPort());
    m_assignVectors->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
}

QString PolyDataAttributeGlyphMapping::name() const
{
    assert(m_dataArray);
    return QString::fromUtf8(m_dataArray->GetName());
}

vtkAlgorithmOutput * PolyDataAttributeGlyphMapping::vectorDataOutputPort()
{
    return m_assignVectors->GetOutputPort();
}
