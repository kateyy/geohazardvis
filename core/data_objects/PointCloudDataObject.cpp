#include "PointCloudDataObject.h"

#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/types.h>
#include <core/rendered_data/RenderedPointCloudData.h>
#include <core/table_model/QVtkTableModelPointCloudData.h>


PointCloudDataObject::PointCloudDataObject(const QString & name, vtkPolyData & dataSet)
    : GenericPolyDataObject(name, dataSet)
{
}

PointCloudDataObject::~PointCloudDataObject() = default;

std::unique_ptr<DataObject> PointCloudDataObject::newInstance(const QString & name, vtkDataSet * dataSet) const
{
    if (auto poly = vtkPolyData::SafeDownCast(dataSet))
    {
        return std::make_unique<PointCloudDataObject>(name, *poly);
    }
    return{};
}

IndexType PointCloudDataObject::defaultAttributeLocation() const
{
    return IndexType::points;
}

std::unique_ptr<RenderedData> PointCloudDataObject::createRendered()
{
    return std::make_unique<RenderedPointCloudData>(*this);
}

void PointCloudDataObject::addDataArray(vtkDataArray & dataArray)
{
    dataSet()->GetPointData()->AddArray(&dataArray);
}

const QString & PointCloudDataObject::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & PointCloudDataObject::dataTypeName_s()
{
    static const QString name{ "Point Cloud" };

    return name;
}

std::unique_ptr<QVtkTableModel> PointCloudDataObject::createTableModel()
{
    auto model = std::make_unique<QVtkTableModelPointCloudData>();
    model->setDataObject(this);

    return std::move(model);
}
