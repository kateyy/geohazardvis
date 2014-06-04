#include "RenderedPolyData.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <vtkPolyData.h>

#include <vtkElevationFilter.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkActor.h>

#include "PolyDataObject.h"

#include "core/Input.h"
#include "core/vtkhelper.h"

#include "gui/widgets/DataChooser.h"


RenderedPolyData::RenderedPolyData(std::shared_ptr<const PolyDataObject> dataObject)
    : RenderedData(dataObject)
{
}

RenderedPolyData::~RenderedPolyData() = default;

std::shared_ptr<const PolyDataObject> RenderedPolyData::polyDataObject() const
{
    assert(dynamic_cast<const PolyDataObject*>(dataObject().get()));
    return std::static_pointer_cast<const PolyDataObject>(dataObject());
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

    return actor;
}

void RenderedPolyData::setSurfaceColorMapping(DataSelection dataSelection, const QImage & gradient)
{
    vtkPolyDataMapper * mapper = createDataMapper(dataSelection, gradient);

    actor()->SetMapper(mapper);
}

vtkPolyDataMapper * RenderedPolyData::createDataMapper(DataSelection dataSelection, const QImage & gradient) const
{
    const PolyDataInput & input = *polyDataObject()->polyDataInput().get();

    vtkPolyDataMapper * mapper = input.createNamedMapper();

    switch (dataSelection)
    {
    case DataSelection::NoSelection:
    case DataSelection::DefaultColor:
        mapper->SetInputData(input.polyData());
        return mapper;
    }

    VTK_CREATE(vtkElevationFilter, elevation);
    elevation->SetInputData(input.polyData());

    float minValue, maxValue;

    switch (dataSelection)
    {
    case DataSelection::Vertex_xValues:
        minValue = input.polyData()->GetBounds()[0];
        maxValue = input.polyData()->GetBounds()[1];
        elevation->SetLowPoint(minValue, 0, 0);
        elevation->SetHighPoint(maxValue, 0, 0);
        break;

    case DataSelection::Vertex_yValues:
        minValue = input.polyData()->GetBounds()[2];
        maxValue = input.polyData()->GetBounds()[3];
        elevation->SetLowPoint(0, minValue, 0);
        elevation->SetHighPoint(0, maxValue, 0);
        break;

    case DataSelection::Vertex_zValues:
        minValue = input.polyData()->GetBounds()[4];
        maxValue = input.polyData()->GetBounds()[5];
        elevation->SetLowPoint(0, 0, minValue);
        elevation->SetHighPoint(0, 0, maxValue);
        break;
    }

    mapper->SetInputConnection(elevation->GetOutputPort());

    // use alpha = 1.0, if the image doesn't have a alpha channel
    int alphaMask = gradient.hasAlphaChannel() ? 0x00 : 0xFF;

    VTK_CREATE(vtkLookupTable, lut);
    lut->SetNumberOfTableValues(gradient.width());
    for (int i = 0; i < gradient.width(); ++i)
    {
        QRgb color = gradient.pixel(i, 0);
        lut->SetTableValue(i, qRed(color) / 255.0, qGreen(color) / 255.0, qBlue(color) / 255.0, (alphaMask | qAlpha(color)) / 255.0);
    }
    lut->SetValueRange(minValue, maxValue);

    mapper->SetLookupTable(lut);

    return mapper;
}
