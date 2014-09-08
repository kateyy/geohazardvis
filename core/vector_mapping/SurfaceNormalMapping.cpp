#include "SurfaceNormalMapping.h"

#include <cassert>

#include <vtkPolyData.h>

#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPoints.h>

#include <vtkGlyph3D.h>
#include <vtkPolyDataNormals.h>

#include <vtkVertexGlyphFilter.h>

#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
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
{
    if (!m_isValid)
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
        vtkDataArray * centroid = polyData()->GetCellData()->GetArray("centroid");

        if (m_normalType == NormalType::CellNormal && centroid)
        {
            VTK_CREATE(vtkPoints, points);
            points->SetData(centroid);

            VTK_CREATE(vtkPolyData, pointsPolyData);
            pointsPolyData->SetPoints(points);

            VTK_CREATE(vtkVertexGlyphFilter, filter);
            filter->SetInputData(pointsPolyData);
            filter->Update();
            vtkPolyData * processedPoints = filter->GetOutput();

            processedPoints->GetPointData()->SetNormals(polyData()->GetCellData()->GetNormals());

            arrowGlyph()->SetInputData(processedPoints);
        }
        else
        {
            arrowGlyph()->SetInputData(polyData());
        }

        m_normalTypeChanged = false;
    }
}
