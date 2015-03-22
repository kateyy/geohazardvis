#include "ImageDataLIC2DMapping.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <vtkAssignAttribute.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkImageDataLIC2D.h>
#include <vtkImageNormalize.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLExtensionManager.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>

#include <core/AbstractVisualizedData.h>
#include <core/vtkhelper.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/NoiseImageSource.h>


const QString ImageDataLIC2DMapping::s_name = "2D Image Data Line Integral Convolution";


const bool ImageDataLIC2DMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<ImageDataLIC2DMapping>);

ImageDataLIC2DMapping::ImageDataLIC2DMapping(const QList<AbstractVisualizedData *> & visualizedData)
    : ColorMappingData(visualizedData)
    , m_noiseImage{}
{
    vtkImageData * image;
    for (AbstractVisualizedData * v : visualizedData)
        for (int i = 0; !m_isValid && i < v->numberOfColorMappingInputs(); ++i)
            if ((image = vtkImageData::SafeDownCast(v->colorMappingInputData(i)))
                && (image->GetPointData()->GetVectors()
                && (image->GetDataDimension() == 2)))
            {
                m_isValid = true;
                break;
            }

    if (!m_isValid)
        return;

    m_noiseImage = vtkSmartPointer<NoiseImageSource>::New();
    m_noiseImage->SetExtent(0, 1023, 0, 1023, 0, 0);
    m_noiseImage->SetNumberOfComponents(1);
    m_noiseImage->SetValueRange(0, 1);
}

ImageDataLIC2DMapping::~ImageDataLIC2DMapping() = default;

QString ImageDataLIC2DMapping::name() const
{
    return "LIC 2D";
}

vtkSmartPointer<vtkAlgorithm> ImageDataLIC2DMapping::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    vtkDataArray * imageVectors = nullptr;
    vtkImageData * image = vtkImageData::SafeDownCast(visualizedData->colorMappingInputData(connection));
    if (image)
        imageVectors = image->GetPointData()->GetVectors();

    if (image->GetDataDimension() != 2 || !imageVectors)
    {
        VTK_CREATE(vtkPassThrough, filter);
        filter->SetInputConnection(visualizedData->colorMappingInput(connection));
        return filter;
    }


    auto & lics = m_lic2D[visualizedData];

    static const std::string scaledVectorsName{ "scaledVectors" };

    if (lics.size() <= connection)
        lics.resize(connection + 1);

    auto & lic = lics[connection];

    if (!lic)
    {
        VTK_CREATE(vtkAssignAttribute, assignScalars);
        assignScalars->SetInputConnection(visualizedData->colorMappingInput(connection));
        assignScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

        VTK_CREATE(vtkImageNormalize, normalize);
        normalize->SetInputConnection(assignScalars->GetOutputPort());

        VTK_CREATE(vtkAssignAttribute, assignScaledVectors);
        assignScaledVectors->SetInputConnection(normalize->GetOutputPort());
        assignScaledVectors->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

        lic = vtkSmartPointer<vtkImageDataLIC2D>::New();
        lic->SetInputConnection(0, assignScaledVectors->GetOutputPort());
        lic->SetInputConnection(1, m_noiseImage->GetOutputPort());
        lic->SetSteps(50);
        lic->GlobalWarningDisplayOff();
        lic->SetContext(glContext());
    }

    return lic;
}

bool ImageDataLIC2DMapping::usesFilter() const
{
    return true;
}

QMap<vtkIdType, QPair<double, double>> ImageDataLIC2DMapping::updateBounds()
{
    return{ { 0, { 0, 1 } } };  // by LIC definition
}

vtkRenderWindow * ImageDataLIC2DMapping::glContext()
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
