#include "VectorGrid3DDataObject.h"

#include <cassert>
#include <limits>

#include <QDebug>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <core/types.h>
#include <core/filters/GeographicTransformationFilter.h>
#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/table_model/QVtkTableModelVectorGrid3D.h>
#include <core/utility/conversions.h>
#include <core/utility/DataExtent.h>


VectorGrid3DDataObject::VectorGrid3DDataObject(const QString & name, vtkImageData & dataSet)
    : CoordinateTransformableDataObject(name, &dataSet)
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

    auto data = dataSet.GetPointData()->GetScalars();
    if (!data)
    {
        data = dataSet.GetPointData()->GetVectors();
        dataSet.GetPointData()->SetScalars(data);
    }

    dataSet.GetExtent(m_extent.data());
}

VectorGrid3DDataObject::~VectorGrid3DDataObject() = default;

std::unique_ptr<DataObject> VectorGrid3DDataObject::newInstance(const QString & name, vtkDataSet * dataSet) const
{
    if (auto image = vtkImageData::SafeDownCast(dataSet))
    {
        return std::make_unique<VectorGrid3DDataObject>(name, *image);
    }
    return{};
}

bool VectorGrid3DDataObject::is3D() const
{
    return true;
}

IndexType VectorGrid3DDataObject::defaultAttributeLocation() const
{
    return IndexType::points;
}

std::unique_ptr<RenderedData> VectorGrid3DDataObject::createRendered()
{
    return std::make_unique<RenderedVectorGrid3D>(*this);
}

void VectorGrid3DDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetPointData()->AddArray(&dataArray);
}

const QString & VectorGrid3DDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & VectorGrid3DDataObject::dataTypeName_s()
{
    static const QString name{ "Regular 3D Grid" };
    return name;
}

vtkImageData & VectorGrid3DDataObject::imageData()
{
    auto ds = dataSet();
    assert(dynamic_cast<vtkImageData *>(ds));
    return static_cast<vtkImageData &>(*ds);
}

const vtkImageData & VectorGrid3DDataObject::imageData() const
{
    auto ds = dataSet();
    assert(dynamic_cast<const vtkImageData *>(ds));
    return static_cast<const vtkImageData &>(*ds);
}

ImageExtent VectorGrid3DDataObject::extent()
{
    assert(ImageExtent(m_extent) == ImageExtent(imageData().GetExtent()));
    return ImageExtent(m_extent);
}

int VectorGrid3DDataObject::numberOfComponents()
{
    return dataSet()->GetPointData()->GetScalars()->GetNumberOfComponents();
}

ValueRange<> VectorGrid3DDataObject::scalarRange(int component)
{
    return ValueRange<>(dataSet()->GetPointData()->GetScalars()->GetRange(component));
}

std::unique_ptr<QVtkTableModel> VectorGrid3DDataObject::createTableModel()
{
    std::unique_ptr<QVtkTableModel> model = std::make_unique<QVtkTableModelVectorGrid3D>();
    model->setDataObject(this);

    return model;
}

bool VectorGrid3DDataObject::checkIfStructureChanged()
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

vtkSmartPointer<vtkAlgorithm> VectorGrid3DDataObject::createTransformPipeline(
    const CoordinateSystemSpecification & toSystem,
    vtkAlgorithmOutput * pipelineUpstream) const
{
    if (!GeographicTransformationFilter::IsTransformationSupported(
        coordinateSystem(), toSystem))
    {
        return{};
    }

    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    filter->SetInputConnection(pipelineUpstream);
    filter->SetTargetCoordinateSystem(toSystem);
    return filter;
}
