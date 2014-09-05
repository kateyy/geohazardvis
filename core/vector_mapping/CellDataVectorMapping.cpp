#include "CellDataVectorMapping.h"

#include <cstring>

#include <vtkPolyData.h>

#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPoints.h>

#include <vtkGlyph3D.h>

#include <vtkVertexGlyphFilter.h>

#include <core/vtkhelper.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "cell data vectors";
}

const bool CellDataVectorMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CellDataVectorMapping>);

using namespace reflectionzeug;

CellDataVectorMapping::CellDataVectorMapping(RenderedData * renderedData)
    : VectorsForSurfaceMapping(renderedData)
{
    arrowGlyph()->SetVectorModeToUseVector();
}

CellDataVectorMapping::~CellDataVectorMapping() = default;

QString CellDataVectorMapping::name() const
{
    return s_name;
}

void CellDataVectorMapping::initialize()
{
    assert(polyData());

    // find first non centroid array
    vtkCellData * cellData = polyData()->GetCellData();
    vtkDataArray * vectorArray = nullptr;

    for (int i = 0; vtkDataArray * a = cellData->GetArray(i); ++i)
    {
        if (a && (std::strncmp(a->GetName(), "centroid", 9)))
        {
            vectorArray = a;
            break;
        }
    }

    // TODO instanced shouldn't be created without vector data available
    if (!vectorArray)
        return;
    
    vtkDataArray * centroid = cellData->GetArray("centroid");
    assert(centroid);   // TODO calculate if needed

    VTK_CREATE(vtkPoints, points);
    points->SetData(centroid);

    VTK_CREATE(vtkPolyData, pointsPolyData);
    pointsPolyData->SetPoints(points);

    VTK_CREATE(vtkVertexGlyphFilter, filter);
    filter->SetInputData(pointsPolyData);
    filter->Update();
    vtkPolyData * processedPoints = filter->GetOutput();

    processedPoints->GetPointData()->SetVectors(vectorArray);

    arrowGlyph()->SetInputData(processedPoints);
}
