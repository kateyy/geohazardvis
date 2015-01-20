#include "VectorField3DLIC2DPlanes.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkImageDataLIC2D.h>
#include <vtkPointData.h>

#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/color_mapping/ColorMappingRegistry.h>


const QString VectorField3DLIC2DPlanes::s_name = "Vector Field Line Integral Convolution Planes";


const bool VectorField3DLIC2DPlanes::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<VectorField3DLIC2DPlanes>);

VectorField3DLIC2DPlanes::VectorField3DLIC2DPlanes(const QList<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
{
    for (AbstractVisualizedData * v : visualizedData)
        if (auto g = dynamic_cast<RenderedVectorGrid3D *>(v))
            m_vectorGrids << g;

    m_isValid = !m_vectorGrids.isEmpty();
}

VectorField3DLIC2DPlanes::~VectorField3DLIC2DPlanes() = default;

QString VectorField3DLIC2DPlanes::name() const
{
    return "LIC 2D";
}

void VectorField3DLIC2DPlanes::configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);
}

QMap<vtkIdType, QPair<double, double>> VectorField3DLIC2DPlanes::updateBounds()
{
    double totalMin = std::numeric_limits<double>::max();
    double totalMax = std::numeric_limits<double>::lowest();

    for (RenderedVectorGrid3D * grid : m_vectorGrids)
    {

        for (int i = 0; i < 3; ++i)
        {
            grid->m_lic2D[i]->Update();
            vtkDataArray * lic = grid->m_lic2D[i]->GetOutput()->GetPointData()->GetScalars();

            double range[2];
            lic->GetRange(range);
            totalMin = std::min(totalMin, range[0]);
            totalMax = std::max(totalMax, range[1]);
        }
    }

    return{ { 0, {totalMin, totalMax} } };
}
