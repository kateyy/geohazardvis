#include "RenderedPolyData.h"

#include <cassert>

#include <QImage>

#include <vtkLookupTable.h>

#include <vtkPolyData.h>

#include <vtkElevationFilter.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkActor.h>

#include "PolyDataObject.h"

#include "core/Input.h"
#include "core/vtkhelper.h"
#include "core/data_mapping/ScalarsForColorMapping.h"



RenderedPolyData::RenderedPolyData(PolyDataObject * dataObject)
    : RenderedData(dataObject)
{
}

RenderedPolyData::~RenderedPolyData() = default;

PolyDataObject * RenderedPolyData::polyDataObject()
{
    assert(dynamic_cast<PolyDataObject*>(dataObject()));
    return static_cast<PolyDataObject *>(dataObject());
}

const PolyDataObject * RenderedPolyData::polyDataObject() const
{
    assert(dynamic_cast<const PolyDataObject*>(dataObject()));
    return static_cast<const PolyDataObject *>(dataObject());
}

vtkProperty * RenderedPolyData::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    prop->SetColor(0, 0.6, 0);
    prop->SetOpacity(1.0);
    prop->SetInterpolationToFlat();
    prop->SetEdgeVisibility(true);
    prop->SetEdgeColor(0.1, 0.1, 0.1);
    prop->SetLineWidth(1.2);
    prop->SetBackfaceCulling(false);
    prop->SetLighting(false);

    return prop;
}

vtkActor * RenderedPolyData::createActor() const
{
    vtkActor * actor = vtkActor::New();
    actor->SetMapper(createDataMapper());

    return actor;
}

void RenderedPolyData::updateScalarToColorMapping()
{
    vtkPolyDataMapper * mapper = createDataMapper();

    actor()->SetMapper(mapper);
}

vtkPolyDataMapper * RenderedPolyData::createDataMapper() const
{
    const PolyDataInput & input = *polyDataObject()->polyDataInput().get();

    vtkPolyDataMapper * mapper = input.createNamedMapper();

    // no mapping: use default colors
    if (!m_scalars || !m_gradient)
    {
        mapper->SetInputData(input.polyData());
        return mapper;
    }

    VTK_CREATE(vtkElevationFilter, elevation);
    elevation->SetInputData(input.polyData());

    float minValue = m_scalars->minValue();
    float maxValue = m_scalars->maxValue();

    // TODO cleanup
    if (m_scalars->name() == "x values")
    {
        elevation->SetLowPoint(minValue, 0, 0);
        elevation->SetHighPoint(maxValue, 0, 0);
    }
    else if (m_scalars->name() == "y values")
    {
        elevation->SetLowPoint(0, minValue, 0);
        elevation->SetHighPoint(0, maxValue, 0);
    }
    else if (m_scalars->name() == "z values")
    {
        elevation->SetLowPoint(0, 0, minValue);
        elevation->SetHighPoint(0, 0, maxValue);
    }
    else
    {
        assert(false);
    }

    mapper->SetInputConnection(elevation->GetOutputPort());

    // use alpha = 1.0, if the image doesn't have a alpha channel
    int alphaMask = m_gradient->hasAlphaChannel() ? 0x00 : 0xFF;

    VTK_CREATE(vtkLookupTable, lut);
    lut->SetNumberOfTableValues(m_gradient->width());
    for (int i = 0; i < m_gradient->width(); ++i)
    {
        QRgb color = m_gradient->pixel(i, 0);
        lut->SetTableValue(i, qRed(color) / 255.0, qGreen(color) / 255.0, qBlue(color) / 255.0, (alphaMask | qAlpha(color)) / 255.0);
    }
    lut->SetValueRange(minValue, maxValue);

    mapper->SetLookupTable(lut);

    return mapper;
}
