#include "CellDataVectorMapping.h"

#include <cstring>

#include <vtkPolyData.h>
#include <vtkPolygon.h>
#include <vtkIdTypeArray.h>
#include <vtkFloatArray.h>

#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellIterator.h>
#include <vtkPoints.h>

#include <vtkGlyph3D.h>

#include <vtkVertexGlyphFilter.h>

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
        if (a && (std::strncmp(a->GetName(), "centroid", 9)))
            vectorArrays << a;
    }

    QList<VectorsForSurfaceMapping *> instances;
    for (vtkDataArray * vectorsArray : vectorArrays)
    {
        CellDataVectorMapping * mapping = new CellDataVectorMapping(renderedData, vectorsArray);
        if (mapping->isValid())
            mapping->initialize();
        instances << mapping;
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

    vtkCellData * cellData = polyData()->GetCellData();
    
    vtkSmartPointer<vtkDataArray> centroids = cellData->GetArray("centroid");
    if (!centroids)
    {
        centroids = vtkSmartPointer<vtkFloatArray>::New();
        centroids->SetName("centroid");
        centroids->SetNumberOfComponents(3);
        centroids->SetNumberOfTuples(polyData()->GetNumberOfCells());
        vtkSmartPointer<vtkCellIterator> it = vtkSmartPointer<vtkCellIterator>::Take(polyData()->NewCellIterator());
        double centroid[3];
        vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
        vtkPoints * polyDataPoints = polyData()->GetPoints();
        vtkIdType centroidIndex = 0;
        for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
        {
            idArray->SetArray(it->GetPointIds()->GetPointer(0), it->GetNumberOfPoints(), true);
            vtkPolygon::ComputeCentroid(
                idArray,
                polyDataPoints,
                centroid);
            centroids->SetTuple(centroidIndex++, centroid);
        }
        cellData->AddArray(centroids);
    }

    VTK_CREATE(vtkPoints, points);
    points->SetData(centroids);

    VTK_CREATE(vtkPolyData, pointsPolyData);
    pointsPolyData->SetPoints(points);

    VTK_CREATE(vtkVertexGlyphFilter, filter);
    filter->SetInputData(pointsPolyData);
    filter->Update();
    vtkPolyData * processedPoints = filter->GetOutput();

    processedPoints->GetPointData()->SetVectors(m_dataArray);

    arrowGlyph()->SetInputData(processedPoints);
}
