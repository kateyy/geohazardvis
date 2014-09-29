#include "SurfaceNormalMapping.h"

#include <cassert>

#include <vtkPolyData.h>

#include <vtkPointData.h>
#include <vtkCellData.h>

#include <vtkGlyph3D.h>
#include <vtkPolyDataNormals.h>

#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "surface normals";
}

const bool SurfaceNormalMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<SurfaceNormalMapping>);

using namespace reflectionzeug;

SurfaceNormalMapping::SurfaceNormalMapping(RenderedData * renderedData)
    : VectorsForSurfaceMapping(renderedData)
    , m_normalType(NormalType::CellNormal)
    , m_normalTypeChanged(true)
    , m_polyData(nullptr)
{
    m_polyData = dynamic_cast<PolyDataObject *>(dataObject());

    if (!m_isValid || !m_polyData)
        return;

    arrowGlyph()->SetVectorModeToUseNormal();
}

SurfaceNormalMapping::~SurfaceNormalMapping() = default;

QString SurfaceNormalMapping::name() const
{
    return s_name;
}

PropertyGroup * SurfaceNormalMapping::createPropertyGroup()
{
    PropertyGroup * group = VectorsForSurfaceMapping::createPropertyGroup();
    group->setName("normals");

    auto * prop_normalType = group->addProperty<NormalType>("type",
        [this](){ return m_normalType;
    },
        [this](NormalType type) { 
        m_normalType = type;
        m_normalTypeChanged = true;
        updateGlyphs();
        emit geometryChanged();
    });
    prop_normalType->setStrings({
            { NormalType::CellNormal, "cell normal" },
            { NormalType::PointNormal, "point normal" }
    });

    return group;
}

void SurfaceNormalMapping::visibilityChangedEvent()
{
    updateGlyphs();
}

void SurfaceNormalMapping::updateGlyphs()
{
    assert(polyData());

    // compute normals if needed
    bool computePointNormals = !polyData()->GetPointData()->HasArray("Normals");
    bool computeCellNormals = !polyData()->GetCellData()->HasArray("Normals");
    if (computePointNormals || computeCellNormals)
    {
        VTK_CREATE(vtkPolyDataNormals, inputNormals);
        inputNormals->SetComputePointNormals(computePointNormals);
        inputNormals->SetComputeCellNormals(computePointNormals);
        inputNormals->SetInputDataObject(polyData());
        inputNormals->Update();

        if (computePointNormals)
            polyData()->GetPointData()->SetNormals(inputNormals->GetOutput()->GetPointData()->GetNormals());
        if (computeCellNormals)
            polyData()->GetCellData()->SetNormals(inputNormals->GetOutput()->GetCellData()->GetNormals());
    }

    if (m_normalTypeChanged)
    {
        if (m_normalType == NormalType::CellNormal)
            arrowGlyph()->SetInputConnection(m_polyData->cellCentersOutputPort());
        else
            arrowGlyph()->SetInputData(polyData());

        m_normalTypeChanged = false;
    }
}
