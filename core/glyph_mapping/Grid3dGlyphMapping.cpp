#include "Grid3dGlyphMapping.h"

#include <cstring>

#include <vtkPointData.h>

#include <vtkGlyph3D.h>

#include <vtkImageData.h>
#include <vtkAssignAttribute.h>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


namespace
{
const QString s_name = "grid 3d vectors";
}

const bool Grid3dGlyphMapping::s_registered = GlyphMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

using namespace reflectionzeug;


QList<GlyphMappingData *> Grid3dGlyphMapping::newInstances(RenderedData * renderedData)
{
    RenderedVectorGrid3D * renderedGrid = dynamic_cast<RenderedVectorGrid3D *>(renderedData);
    if (!renderedGrid)
        return{};

    vtkPointData * pointData = renderedGrid->resampledDataSet()->GetPointData();
    QList<vtkDataArray *> vectorArrays;
    for (int i = 0; vtkDataArray * a = pointData->GetArray(i); ++i)
    {
        assert(a);

        if (a->GetInformation()->Has(DataObject::ArrayIsAuxiliaryKey())
            && a->GetInformation()->Get(DataObject::ArrayIsAuxiliaryKey()))
            continue;

        vectorArrays << a;
    }

    QList<GlyphMappingData *> instances;
    for (vtkDataArray * vectorArray : vectorArrays)
    {
        Grid3dGlyphMapping * mapping = new Grid3dGlyphMapping(renderedGrid, vectorArray);
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

Grid3dGlyphMapping::Grid3dGlyphMapping(RenderedVectorGrid3D * renderedGrid, vtkDataArray * dataArray)
    : GlyphMappingData(renderedGrid)
    , m_renderedGrid(renderedGrid)
    , m_dataArray(dataArray)
{
    setVisible(true);

    arrowGlyph()->SetVectorModeToUseVector();
    setRepresentation(Representation::SimpleArrow);
    setColor(1, 0, 0);
    updateArrowLength();

    connect(renderedGrid, &RenderedVectorGrid3D::sampleRateChanged, [this] (int, int, int) {
        this->updateArrowLength(); });
}

QString Grid3dGlyphMapping::name() const
{
    assert(m_dataArray);
    return QString::fromUtf8(m_dataArray->GetName());
}

void Grid3dGlyphMapping::initialize()
{
    VTK_CREATE(vtkAssignAttribute, assignAttribute);
    assignAttribute->SetInputConnection(m_renderedGrid->resampledOuputPort());
    assignAttribute->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    arrowGlyph()->SetInputConnection(assignAttribute->GetOutputPort());
}

void Grid3dGlyphMapping::updateArrowLength()
{
    double cellSpacing = m_renderedGrid->resampledDataSet()->GetSpacing()[0];
    arrowGlyph()->SetScaleFactor(0.75 * cellSpacing);

    emit geometryChanged();
}
