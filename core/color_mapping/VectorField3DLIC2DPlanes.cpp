#include "VectorField3DLIC2DPlanes.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <vtkArrayCalculator.h>
#include <vtkAssignAttribute.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLExtensionManager.h>
#include <vtkPointData.h>

#include <core/vtkhelper.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/NoiseImageSource.h>
#include <core/filters/vtkImageDataLIC2D.h>
#include <core/rendered_data/RenderedVectorGrid3D.h>


const QString VectorField3DLIC2DPlanes::s_name = "Vector Field Line Integral Convolution Planes";


const bool VectorField3DLIC2DPlanes::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<VectorField3DLIC2DPlanes>);

VectorField3DLIC2DPlanes::VectorField3DLIC2DPlanes(const QList<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
    , m_noiseImage(vtkSmartPointer<NoiseImageSource>::New())
{
    for (AbstractVisualizedData * v : visualizedData)
        if (auto g = dynamic_cast<RenderedVectorGrid3D *>(v))
            m_vectorGrids << g;

    m_isValid = !m_vectorGrids.isEmpty();

    m_noiseImage->SetExtent(0, 1023, 0, 1023, 0, 0);
    m_noiseImage->SetNumberOfComponents(1);
    m_noiseImage->SetValueRange(0, 1);
}

VectorField3DLIC2DPlanes::~VectorField3DLIC2DPlanes() = default;

QString VectorField3DLIC2DPlanes::name() const
{
    return "LIC 2D";
}

vtkSmartPointer<vtkAlgorithm> VectorField3DLIC2DPlanes::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    auto & lics = m_lic2D[visualizedData];

    static const std::string scaledVectorsName{ "scaledVectors" };

    if (lics.size() <= connection)
        lics.resize(connection + 1);

    auto & lic = lics[connection];

    if (!lic)
    {
        auto vectors = visualizedData->colorMappingInputData(connection)->GetPointData()->GetVectors();
        assert(vectors);

        double vectorRange[2] = {
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::lowest() };
        for (int i = 0; i < 3; ++i)
        {
            double r[2];
            vectors->GetRange(r, i);
            vectorRange[0] = std::min(vectorRange[0], r[0]);
            vectorRange[1] = std::max(vectorRange[1], r[1]);
        }

        double vectorScaleFactor = 1.0 / std::max(std::abs(vectorRange[0]), std::abs(vectorRange[1]));

        VTK_CREATE(vtkArrayCalculator, vectorScale);
        vectorScale->SetInputConnection(visualizedData->colorMappingInput(connection));
        vectorScale->AddVectorArrayName(vectors->GetName());
        vectorScale->SetResultArrayName(scaledVectorsName.c_str());
        vectorScale->SetFunction((std::to_string(vectorScaleFactor) + "*" + vectors->GetName()).c_str());

        VTK_CREATE(vtkAssignAttribute, assignScaledVectors);
        assignScaledVectors->SetInputConnection(vectorScale->GetOutputPort());
        assignScaledVectors->Assign(scaledVectorsName.c_str(),
            vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

        lic = vtkSmartPointer<vtkImageDataLIC2D>::New();
        lic->SetInputConnection(0, assignScaledVectors->GetOutputPort());
        lic->SetInputConnection(1, m_noiseImage->GetOutputPort());
        lic->SetSteps(50);
        lic->GlobalWarningDisplayOff();
        lic->SetContext(glContext());
    }

    return lic;
}

bool VectorField3DLIC2DPlanes::usesFilter() const
{
    return true;
}

QMap<vtkIdType, QPair<double, double>> VectorField3DLIC2DPlanes::updateBounds()
{
    return{ { 0, { 0, 1 } } };  // by LIC definition
}

vtkRenderWindow * VectorField3DLIC2DPlanes::glContext()
{
    if (m_glContext)
        return m_glContext;

    m_glContext = vtkSmartPointer<vtkRenderWindow>::New();
    vtkOpenGLRenderWindow * openGLContext = vtkOpenGLRenderWindow::SafeDownCast(m_glContext);
    assert(openGLContext);
    openGLContext->GetExtensionManager()->IgnoreDriverBugsOn(); // required for Intel HD
    openGLContext->SetMultiSamples(1);  // multi sampling is not implemented for off screen contexts
    openGLContext->OffScreenRenderingOn();

    return m_glContext;
}
