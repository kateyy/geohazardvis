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

#pragma once

#include <memory>
#include <utility>

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <QString>

#include <core/types.h>
#include <core/CoordinateSystems.h>


class vtkDataArray;
class DataObject;


class CORE_API DataSetResidualHelper
{
public:
    DataSetResidualHelper();
    virtual ~DataSetResidualHelper();

    void setObservationDataObject(DataObject * observationDataObject);
    DataObject * observationDataObject();
    void setObservationScalars(const QString & name, IndexType location);
    void setObservationScalarsScale(double scale);
    double observationScalarsScale() const;

    void setModelDataObject(DataObject * modelDataObject);
    DataObject * modelDataObject();
    void setModelScalars(const QString & name, IndexType location);
    void setModelScalarsScale(double scale);
    double modelScalarsScale() const;

    /**
     * Specify the name for new residual data objects.
     * This must be set before calling updateResidual().
     */
    void setResidualDataObjectName(const QString & name);
    const QString & residualDataObjectName() const;

    DataObject * residualDataObject();
    std::unique_ptr<DataObject> takeResidualDataObject();

    enum class InputData
    {
        Observation,
        Model
    };

    void setGeometrySource(InputData inputData);
    InputData geometrySource() const;

    void setTargetCoordinateSystem(const CoordinateSystemSpecification & spec);
    const CoordinateSystemSpecification & targetCoordinateSystem() const;

    /**
     * Specify the line-of-sight of the observing satellite by its incidence angle (alpha) and
     * heading (theta), both in degrees.
     *
     * A line-of-sight vector will be derived from these angles, and displacement vectors of model
     * or observation data will be transformed to LOS-displacements based on this vector.
     * Both angles default to 0 degrees, which is a LOS-vector of (0, 0, 1).
     */
    void setDeformationLineOfSight(double incidenceAngleDeg, double satelliteHeadingDeg);
    double losIncidenceAngleDegrees() const;
    double losSatelliteHeadingDegrees() const;

    void setProjectedAttributeNameSuffix(const QString & suffix);
    const QString & projectedAttributeNameSuffix() const;

    bool isSetupComplete() const;

    /**
     * Project deformation vectors to line of sight displacements, where applicable.
     * If projects are applied, the projected scalars will be added to the observation/model
     * point or cell attribute so that the projected scalars can be used in visualization.
     * @return whether observation and model displacements are valid.
     */
    bool projectDisplacementsToLineOfSight();
    /**
     * Compute the residual.
     * This will only have an effect if isSetupComplete() returns true.
     */
    bool updateResidual();

    const QString & losObservationScalarsName() const;
    const QString & losModelScalarsName() const;

private:
    void invalidateResults();
    void invalidateResidual();

private:
    struct ScalarDef
    {
        ScalarDef();
        bool isComplete() const;
        QString name;
        IndexType location;
        double scale;
        QString projectedName;
        vtkMTimeType sourceArrayMTime;
        vtkSmartPointer<vtkDataArray> losDisplacements;
    };

    DataObject * m_observationDataObject;
    ScalarDef m_observationScalars;
    DataObject * m_modelDataObject;
    ScalarDef m_modelScalars;

    QString m_residualDataObjectName;
    std::unique_ptr<DataObject> m_residualDataObject;

    InputData m_geometrySource;
    CoordinateSystemSpecification m_targetCoordinateSystem;
    double m_losIncidenceAngleDegrees;
    double m_losSatelliteHeadingDegrees;
    QString m_projectedAttributeSuffix;
};
