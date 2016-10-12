#include "ImageDataObject.h"

#include <cassert>
#include <limits>

#include <QDebug>

#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <core/CoordinateSystems.h>
#include <core/filters/SimpleImageGeoCoordinateTransformFilter.h>
#include <core/rendered_data/RenderedImageData.h>
#include <core/table_model/QVtkTableModelImage.h>
#include <core/utility/conversions.h>
#include <core/utility/DataExtent.h>


ImageDataObject::ImageDataObject(const QString & name, vtkImageData & dataSet)
    : CoordinateTransformableDataObject(name, &dataSet)
    , m_extent{}
{
    vtkVector3d spacing;
    bool spacingChanged = false;
    dataSet.GetSpacing(spacing.GetData());
    for (int i = 0; i < 3; ++i)
    {
        if (spacing[i] < std::numeric_limits<double>::epsilon())
        {
            spacing[i] = 1;
            spacingChanged = true;
        }
    }

    if (spacingChanged)
    {
        qDebug() << "Fixing invalid image spacing in " << name << "(" + 
            vector3ToString(vtkVector3d(dataSet.GetSpacing())) + " to " + vector3ToString(spacing) + ")";
        dataSet.SetSpacing(spacing.GetData());
    }

    if (!dataSet.GetPointData()->GetScalars())
    {
        dataSet.AllocateScalars(VTK_FLOAT, 1);
    }

    dataSet.GetExtent(m_extent.data());
}

ImageDataObject::~ImageDataObject() = default;

bool ImageDataObject::is3D() const
{
    return false;
}

std::unique_ptr<RenderedData> ImageDataObject::createRendered()
{
    return std::make_unique<RenderedImageData>(*this);
}

void ImageDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetPointData()->AddArray(&dataArray);
}

const QString & ImageDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & ImageDataObject::dataTypeName_s()
{
    static const QString name{ "Regular 2D Grid" };
    return name;
}

vtkImageData & ImageDataObject::imageData()
{
    auto img = dataSet();
    assert(dynamic_cast<vtkImageData *>(img));
    return static_cast<vtkImageData &>(*img);
}

const vtkImageData & ImageDataObject::imageData() const
{
    auto img = dataSet();
    assert(dynamic_cast<const vtkImageData *>(img));
    return static_cast<const vtkImageData &>(*img);
}

vtkDataArray & ImageDataObject::scalars()
{
    auto s = dataSet()->GetPointData()->GetScalars();
    assert(s && s->GetName());
    return *s;
}

ImageExtent ImageDataObject::extent()
{
    assert(ImageExtent(m_extent) == ImageExtent(imageData().GetExtent()));
    return ImageExtent(m_extent);
}

ValueRange<> ImageDataObject::scalarRange()
{
    return ValueRange<>(imageData().GetScalarRange());
}

std::unique_ptr<QVtkTableModel> ImageDataObject::createTableModel()
{
    std::unique_ptr<QVtkTableModel> model = std::make_unique<QVtkTableModelImage>();
    model->setDataObject(this);

    return model;
}

bool ImageDataObject::checkIfStructureChanged()
{
    const auto superclassResult = CoordinateTransformableDataObject::checkIfStructureChanged();

    decltype(m_extent) newExtent;
    imageData().GetExtent(newExtent.data());

    bool changed = newExtent != m_extent;

    if (changed)
    {
        m_extent = newExtent;
    }

    return superclassResult || changed;
}

vtkSmartPointer<vtkAlgorithm> ImageDataObject::createTransformPipeline(
    const CoordinateSystemSpecification & toSystem,
    vtkAlgorithmOutput * pipelineUpstream) const
{
    if (coordinateSystem().geographicSystem != "WGS 84"
        || toSystem.geographicSystem != "WGS 84"
        || coordinateSystem().globalMetricSystem != "UTM"
        || toSystem.globalMetricSystem != "UTM"
        || !coordinateSystem().isReferencePointValid())
    {
        return nullptr;
    }

    if (coordinateSystem().type == CoordinateSystemType::geographic
        && toSystem.type == CoordinateSystemType::metricLocal)
    {
        auto filter = vtkSmartPointer<SimpleImageGeoCoordinateTransformFilter>::New();
        filter->SetInputConnection(pipelineUpstream);
        filter->SetTargetCoordinateSystem(SimpleImageGeoCoordinateTransformFilter::LOCAL_METRIC);
        return filter;
    }

    if (coordinateSystem().type == CoordinateSystemType::metricLocal
        && toSystem.type == CoordinateSystemType::geographic)
    {
        auto filter = vtkSmartPointer<SimpleImageGeoCoordinateTransformFilter>::New();
        filter->SetInputConnection(pipelineUpstream);
        filter->SetTargetCoordinateSystem(SimpleImageGeoCoordinateTransformFilter::GLOBAL_GEOGRAPHIC);
        return filter;
    }

    return nullptr;
}
