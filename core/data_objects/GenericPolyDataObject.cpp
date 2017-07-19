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

#include "GenericPolyDataObject.h"

#include <cassert>

#include <QDebug>

#include <vtkAlgorithmOutput.h>
#include <vtkExecutive.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <core/CoordinateSystems.h>
#include <core/types.h>
#include <core/data_objects/DataObject_private.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/filters/AssignPointAttributeToCoordinatesFilter.h>
#include <core/filters/GeographicTransformationFilter.h>
#include <core/filters/SetCoordinateSystemInformationFilter.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{


vtkDataArray * findCoordinatesArrayIgnoreUnit(
    vtkPointSet & dataSet,
    const CoordinateSystemSpecification & coordsSpec)
{
    auto & pointData = *dataSet.GetPointData();
    const auto numArrays = pointData.GetNumberOfArrays();
    for (int i = 0; i < numArrays; ++i)
    {
        auto array = pointData.GetArray(i);
        if (!array || !array->GetName())
        {
            continue;
        }
        auto spec = CoordinateSystemSpecification::fromInformation(*array->GetInformation());
        spec.unitOfMeasurement = coordsSpec.unitOfMeasurement;
        if (spec.isValid() && (spec == coordsSpec))
        {
            return array;
        }
    }

    return nullptr;
}


}

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

bool GenericPolyDataObject::checkIfStructureChanged()
{
    const bool superclassResult = CoordinateTransformableDataObject::checkIfStructureChanged();

    return superclassResult || dPtr().m_inCopyStructure;
}

vtkSmartPointer<vtkAlgorithm> GenericPolyDataObject::createTransformPipeline(
    const CoordinateSystemSpecification & toSystem,
    vtkAlgorithmOutput * pipelineUpstream) const
{
    vtkSmartPointer<vtkAlgorithm> upstream = pipelineUpstream->GetProducer();
    int upstreamPort = pipelineUpstream->GetIndex();
    ReferencedCoordinateSystemSpecification currentSpec = coordinateSystem();

    if (!upstream->GetExecutive()->Update(upstreamPort))
    {
        qWarning() << "Error in pipeline update in" << name();
        return{};
    }
    // Check if there are stored point coordinates
    auto upstreamPoly = vtkPolyData::SafeDownCast(upstream->GetOutputDataObject(upstreamPort));
    if (upstreamPoly)
    {
        // Check if there are stored coordinates in the requested system.
        auto storedCoords = findCoordinatesArrayIgnoreUnit(*upstreamPoly, toSystem);
        if (storedCoords)
        {
            auto && storedCoordsSpec = ReferencedCoordinateSystemSpecification::fromInformation(
                *storedCoords->GetInformation());
            auto assignCoords = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
            assignCoords->SetInputConnection(upstream->GetOutputPort(upstreamPort));
            assignCoords->SetAttributeArrayToAssign(storedCoords->GetName());

            auto setCoordsSpec = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
            setCoordsSpec->SetCoordinateSystemSpec(storedCoordsSpec);
            setCoordsSpec->SetInputConnection(assignCoords->GetOutputPort());

            // Already in the target system? (Unit conversion might still be required)
            if (storedCoordsSpec == toSystem)
            {
                return setCoordsSpec;
            }
            upstream = setCoordsSpec;
            upstreamPort = 0;
            currentSpec = storedCoordsSpec;
        }
        // Just a metric global->local transformation based on stored global metric coordinates?
        // Simplify that by assigning global metric coordinates if available.
        else if (toSystem.type == CoordinateSystemType::metricLocal
            && currentSpec.type != CoordinateSystemType::metricLocal
            && currentSpec.type != CoordinateSystemType::metricGlobal)
        {
            auto globalCoordsCheckSpec = toSystem;
            globalCoordsCheckSpec.type = CoordinateSystemType::metricGlobal;
            if (auto globalCoords = findCoordinatesArrayIgnoreUnit(*upstreamPoly, globalCoordsCheckSpec))
            {
                const auto globalCoordsSpec = ReferencedCoordinateSystemSpecification::fromInformation(
                        *globalCoords->GetInformation());

                auto assignGlobalCoords = vtkSmartPointer<AssignPointAttributeToCoordinatesFilter>::New();
                assignGlobalCoords->SetInputConnection(upstream->GetOutputPort(upstreamPort));
                assignGlobalCoords->SetAttributeArrayToAssign(globalCoords->GetName());

                auto setCoordsSpec = vtkSmartPointer<SetCoordinateSystemInformationFilter>::New();
                setCoordsSpec->SetCoordinateSystemSpec(globalCoordsSpec);
                setCoordsSpec->SetInputConnection(assignGlobalCoords->GetOutputPort());

                upstream = setCoordsSpec;
                upstreamPort = 0;
                currentSpec = globalCoordsSpec;

                // Transformation from stored global to local coordinates is done in downstream.
            }
        }
    }

    // If the target system could not be generated using stored coordinates, support from the
    // transform filter is required.
    if (!GeographicTransformationFilter::IsTransformationSupported(currentSpec, toSystem))
    {
        return{};
    }

    // This transforms between local metric coordinates and geographic coordinates,
    // and/or transforms metric coordinate units.
    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    filter->SetInputConnection(upstream->GetOutputPort(upstreamPort));
    filter->SetTargetCoordinateSystem(toSystem);
    return filter;
}
