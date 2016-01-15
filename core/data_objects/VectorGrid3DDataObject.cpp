#include "VectorGrid3DDataObject.h"

#include <limits>

#include <QDebug>

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/table_model/QVtkTableModelVectorGrid3D.h>
#include <core/utility/conversions.h>


VectorGrid3DDataObject::VectorGrid3DDataObject(const QString & name, vtkImageData & dataSet)
    : DataObject(name, &dataSet)
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

    vtkDataArray * data = dataSet.GetPointData()->GetScalars();
    if (!data)
    {
        data = dataSet.GetPointData()->GetVectors();
        dataSet.GetPointData()->SetScalars(data);
    }

    dataSet.GetExtent(m_extent.data());
}

VectorGrid3DDataObject::~VectorGrid3DDataObject() = default;

bool VectorGrid3DDataObject::is3D() const
{
    return true;
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
    static const QString name{ "3D vector grid" };
    return name;
}

vtkImageData * VectorGrid3DDataObject::imageData()
{
    return static_cast<vtkImageData *>(dataSet());
}

const vtkImageData * VectorGrid3DDataObject::imageData() const
{
    return static_cast<const vtkImageData *>(dataSet());
}

const int * VectorGrid3DDataObject::dimensions()
{
    return static_cast<vtkImageData *>(dataSet())->GetDimensions();
}

const int * VectorGrid3DDataObject::extent()
{
    return static_cast<vtkImageData *>(dataSet())->GetExtent();
}

int VectorGrid3DDataObject::numberOfComponents()
{
    return dataSet()->GetPointData()->GetScalars()->GetNumberOfComponents();
}

const double * VectorGrid3DDataObject::scalarRange(int component)
{
    return dataSet()->GetPointData()->GetScalars()->GetRange(component);
}

std::unique_ptr<QVtkTableModel> VectorGrid3DDataObject::createTableModel()
{
    std::unique_ptr<QVtkTableModel> model = std::make_unique<QVtkTableModelVectorGrid3D>();
    model->setDataObject(this);

    return model;
}

bool VectorGrid3DDataObject::checkIfStructureChanged()
{
    if (DataObject::checkIfStructureChanged())
    {
        return true;
    }

    decltype(m_extent) newExtent;
    imageData()->GetExtent(newExtent.data());

    bool changed = newExtent != m_extent;

    if (changed)
    {
        m_extent = newExtent;
    }

    return changed;
}