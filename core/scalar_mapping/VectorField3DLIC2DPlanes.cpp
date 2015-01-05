#include "VectorField3DLIC2DPlanes.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkImageDataLIC2D.h>
#include <vtkPointData.h>

#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


const QString VectorField3DLIC2DPlanes::s_name = "Vector Field Line Integral Convolution Planes";


const bool VectorField3DLIC2DPlanes::s_isRegistered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<VectorField3DLIC2DPlanes>);

VectorField3DLIC2DPlanes::VectorField3DLIC2DPlanes(const QList<AbstractVisualizedData *> & visualizedData)
    : ScalarsForColorMapping(visualizedData)
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
    ScalarsForColorMapping::configureMapper(visualizedData, mapper);
}

void VectorField3DLIC2DPlanes::updateBounds()
{
    double totalRange[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };

    for (RenderedVectorGrid3D * grid : m_vectorGrids)
    {

        for (int i = 0; i < 3; ++i)
        {
            grid->m_lic2D[i]->Update();
            vtkDataArray * lic = grid->m_lic2D[i]->GetOutput()->GetPointData()->GetScalars();

            double range[2];
            lic->GetRange(range);
            totalRange[0] = std::min(totalRange[0], range[0]);
            totalRange[1] = std::max(totalRange[1], range[1]);
        }
    }

    setDataMinMaxValue(totalRange, 0);
}
