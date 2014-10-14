#include "SurfaceNormalMapping.h"

#include <vtkGlyph3D.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/vector_mapping/VectorMappingRegistry.h>


namespace
{
const QString s_name = "surface normals";
}

const bool SurfaceNormalMapping::s_registered = VectorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<SurfaceNormalMapping>);

using namespace reflectionzeug;

SurfaceNormalMapping::SurfaceNormalMapping(RenderedData * renderedData)
    : VectorMappingData(renderedData)
{
    PolyDataObject * polyData = dynamic_cast<PolyDataObject *>(dataObject());

    if (!m_isValid || !polyData)
        return;

    arrowGlyph()->SetInputConnection(polyData->cellCentersOutputPort());
    arrowGlyph()->SetVectorModeToUseNormal();
}

SurfaceNormalMapping::~SurfaceNormalMapping() = default;

QString SurfaceNormalMapping::name() const
{
    return s_name;
}
