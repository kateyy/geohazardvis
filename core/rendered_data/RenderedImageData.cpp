#include "RenderedImageData.h"

#include <cassert>

#include <vtkInformation.h>

#include <vtkPropCollection.h>

#include <vtkLookupTable.h>

#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageProperty.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>


using namespace reflectionzeug;

namespace
{
    enum Interpolation
    {
        nearest = VTK_NEAREST_INTERPOLATION,
        linear = VTK_LINEAR_INTERPOLATION,
        cubic = VTK_CUBIC_INTERPOLATION
    };
}

RenderedImageData::RenderedImageData(ImageDataObject * dataObject)
    : RenderedData(ContentType::Rendered2D, dataObject)
    , m_mapper(vtkSmartPointer<vtkImageSliceMapper>::New()) // replace with vtkImageResliceMapper?
{
    m_mapper->SetInputConnection(dataObject->processedOutputPort());

    vtkInformation * mapperInfo = m_mapper->GetInformation();
    mapperInfo->Set(DataObject::NameKey(), dataObject->name().toUtf8().data());
    DataObject::setDataObject(mapperInfo, dataObject);
}

reflectionzeug::PropertyGroup * RenderedImageData::createConfigGroup()
{
    PropertyGroup * renderSettings = new PropertyGroup();

    auto prop_interpolation = renderSettings->addProperty<Interpolation>("Interpolation",
        [this] () {
        return static_cast<Interpolation>(property()->GetInterpolationType());
    },
        [this] (Interpolation interpolation) {
        property()->SetInterpolationType(static_cast<int>(interpolation));
        emit geometryChanged();
    });
    prop_interpolation->setStrings({
            { Interpolation::nearest, "nearest" },
            { Interpolation::linear, "linear" },
            { Interpolation::cubic, "cubic" }
    });

    return renderSettings;
}

vtkSmartPointer<vtkPropCollection> RenderedImageData::fetchViewProps()
{
    VTK_CREATE(vtkPropCollection, props);
    props->AddItem(slice());

    return props;
}

vtkImageSlice * RenderedImageData::slice()
{
    if (!m_slice)
    {
        m_slice = vtkSmartPointer<vtkImageSlice>::New();
        m_slice->SetMapper(m_mapper);
        m_slice->SetProperty(property());
    }

    return m_slice;
}

vtkImageProperty * RenderedImageData::property()
{
    if (!m_property)
    {
        m_property = vtkSmartPointer<vtkImageProperty>::New();
        m_property->UseLookupTableScalarRangeOn();
    }

    return m_property;
}

void RenderedImageData::colorMappingGradientChangedEvent()
{
    property()->SetLookupTable(m_gradient);
}

void RenderedImageData::visibilityChangedEvent(bool visible)
{
    slice()->SetVisibility(visible);
}
