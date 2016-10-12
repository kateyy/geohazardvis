#include "GenericPolyDataObject.h"

#include <cassert>

#include <QDebug>

#include <vtkPolyData.h>

#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>


GenericPolyDataObject::GenericPolyDataObject(const QString & name, vtkPolyData & dataSet)
    : CoordinateTransformableDataObject(name, &dataSet)
{
}

GenericPolyDataObject::~GenericPolyDataObject() = default;

bool GenericPolyDataObject::is3D() const
{
    return true;
}

vtkPolyData & GenericPolyDataObject::polyDataSet()
{
    auto ds = dataSet();
    assert(dynamic_cast<vtkPolyData *>(ds));
    return static_cast<vtkPolyData &>(*ds);
}

const vtkPolyData & GenericPolyDataObject::polyDataSet() const
{
    auto ds = dataSet();
    assert(dynamic_cast<const vtkPolyData *>(ds));
    return static_cast<const vtkPolyData &>(*ds);
}

std::unique_ptr<GenericPolyDataObject> GenericPolyDataObject::createInstance(const QString & name, vtkPolyData & dataSet)
{
    const bool hasLines = 0 != dataSet.GetNumberOfLines();
    const bool hasStrips = 0 != dataSet.GetNumberOfStrips();

    if (hasStrips || hasLines)
    {
        qWarning() << "GenericPolyDataObject: unsupported cell types in data set """ + name + """.";
        return{};
    }

    const bool hasVerts = 0 != dataSet.GetNumberOfVerts();
    const bool hasPolys = 0 != dataSet.GetNumberOfPolys();

    if (!hasVerts && !hasPolys)
    {
        qWarning() << "GenericPolyDataObject: no supported cell types in data set """ + name + """.";
        return{};
    }

    if (hasVerts && hasPolys)
    {
        qWarning() << "GenericPolyDataObject: mixed cell types are not supported (""" + name + """).";
        return{};
    }

    if (dataSet.GetNumberOfCells() !=
        dataSet.GetVerts()->GetNumberOfCells()
        + dataSet.GetPolys()->GetNumberOfCells()
        + dataSet.GetStrips()->GetNumberOfCells()
        + dataSet.GetLines()->GetNumberOfCells())
    {
        qWarning() << "GenericPolyDataObject: data set contains unknown cell type (""" + name + """).";
        return{};
    }
    
    if (hasPolys)
    {
        return std::make_unique<PolyDataObject>(name, dataSet);
    }

    if (hasVerts)
    {
        return std::make_unique<PointCloudDataObject>(name, dataSet);
    }

    assert(false);

    return{};
}

double GenericPolyDataObject::pointCoordinateComponent(vtkIdType pointId, int component, bool * validIdPtr)
{
    auto & points = *polyDataSet().GetPoints()->GetData();

    const auto isValid = pointId < points.GetNumberOfTuples()
        && component < points.GetNumberOfComponents();

    if (validIdPtr)
    {
        *validIdPtr = isValid;
    }

    if (!isValid)
    {
        return{};
    }

    return points.GetComponent(pointId, component);
}

bool GenericPolyDataObject::setPointCoordinateComponent(vtkIdType pointId, int component, double value)
{
    auto & points = *polyDataSet().GetPoints()->GetData();

    if (pointId >= points.GetNumberOfTuples()
        || component >= points.GetNumberOfComponents())
    {
        return false;
    }

    points.SetComponent(pointId, component, value);
    polyDataSet().Modified();

    return true;
}
