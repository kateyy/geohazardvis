/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ImageDataLIC2DMapping.h"

#include <algorithm>
#include <cassert>

#include <QVector>

#include <vtkAssignAttribute.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkImageDataLIC2D.h>
#include <vtkImageNormalize.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>

#include <core/AbstractVisualizedData.h>
#include <core/config.h>
#include <core/types.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/data_objects/DataObject.h>
#include <core/filters/NoiseImageSource.h>
#include <core/utility/DataExtent.h>

#if VTK_RENDERING_BACKEND == 1
#include <vtkOpenGLExtensionManager.h>
#endif


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
    {
        for (int i = 0; !m_isValid && i < v->numberOfColorMappingInputs(); ++i)
        {
            if ((image = vtkImageData::SafeDownCast(v->colorMappingInputData(i)))
                && (image->GetPointData()->GetVectors()
                    && (image->GetDataDimension() == 2)))
            {
                m_isValid = true;
                break;
            }
        }
    }

    if (!m_isValid)
    {
        return;
    }

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

QString ImageDataLIC2DMapping::scalarsName(AbstractVisualizedData & vis) const
{
    auto vectors = vis.dataObject().processedDataSet()->GetPointData()->GetVectors();
    assert(vectors);
    return QString::fromUtf8(vectors->GetName());
}

IndexType ImageDataLIC2DMapping::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return IndexType::points;
}

vtkSmartPointer<vtkAlgorithm> ImageDataLIC2DMapping::createFilter(AbstractVisualizedData & visualizedData, int connection)
{
    vtkDataArray * imageVectors = nullptr;
    vtkImageData * image = vtkImageData::SafeDownCast(visualizedData.colorMappingInputData(connection));

    if (image)
        imageVectors = image->GetPointData()->GetVectors();

    if (!image || image->GetDataDimension() != 2 || !imageVectors)
    {
        auto filter = vtkSmartPointer<vtkPassThrough>::New();
        filter->SetInputConnection(visualizedData.colorMappingInput(connection));
        return filter;
    }


    auto & lics = m_lic2D[&visualizedData];

    if (lics.size() <= connection)
        lics.resize(connection + 1);

    auto & lic = lics[connection];

    if (!lic)
    {
        auto assignScalars = vtkSmartPointer<vtkAssignAttribute>::New();
        assignScalars->SetInputConnection(visualizedData.colorMappingInput(connection));
        assignScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

        auto normalize = vtkSmartPointer<vtkImageNormalize>::New();
        normalize->SetInputConnection(assignScalars->GetOutputPort());

        auto assignScaledVectors = vtkSmartPointer<vtkAssignAttribute>::New();
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

std::vector<ValueRange<>> ImageDataLIC2DMapping::updateBounds()
{
    return{ decltype(updateBounds())::value_type({ 0.0, 1.0 }) };  // by LIC definition
}

vtkRenderWindow * ImageDataLIC2DMapping::glContext()
{
    if (m_glContext)
        return m_glContext;

    m_glContext = vtkSmartPointer<vtkRenderWindow>::New();
    vtkOpenGLRenderWindow * openGLContext = vtkOpenGLRenderWindow::SafeDownCast(m_glContext);
    assert(openGLContext);
#if VTK_RENDERING_BACKEND == 1
    openGLContext->GetExtensionManager()->IgnoreDriverBugsOn(); // required for Intel HD
#endif
    openGLContext->SetMultiSamples(1);  // multi sampling is not implemented for off screen contexts
    openGLContext->OffScreenRenderingOn();

    return m_glContext;
}
