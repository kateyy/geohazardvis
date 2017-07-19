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

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/DataExtent_fwd.h>
#include <core/utility/GeographicTransformationUtil.h>


class vtkAlgorithm;
class vtkLineSource;
class vtkTransformPolyDataFilter;
class vtkWarpScalar;

enum class IndexType;
class LineOnCellsSelector2D;
class LineOnPointsSelector2D;


/**
 * Probes the source data along a line defined by two points on the XY-plane and creates a plot for
 * the interpolated data.
 */
class CORE_API DataProfile2DDataObject : public DataObject
{
    Q_OBJECT

public:
    struct CORE_API PreprocessingPipeline
    {
        PreprocessingPipeline(vtkAlgorithm * head = nullptr, vtkAlgorithm * tail = nullptr);
        ~PreprocessingPipeline();
        vtkSmartPointer<vtkAlgorithm> head;
        vtkSmartPointer<vtkAlgorithm> tail;
    };

    /** Create profile with given specifications
     * @param name Object Name, see DataObject API
     * @param sourceData Source data object that will be probed. The DataObject will only be used
     *  in the constructor, only shared VTK data sets are referenced later.
     * @param scalarsName Scalars to probe in the source data object
     * @param scalarsLocation Specifies whether to probe point or cell scalars
     * @param vectorComponent For multi component scalars/vectors, specify which component will be
     *  extracted
     * @param preprocessingPipeline Optional preprocessing pipeline. Will be injected between the
     *  processed/transformed output of the sourceData and the profile transformations.
     */
    DataProfile2DDataObject(
        const QString & name,
        DataObject & sourceData,
        const QString & scalarsName,
        IndexType scalarsLocation,
        vtkIdType vectorComponent,
        const PreprocessingPipeline & preprocessingPipeline = {});
    ~DataProfile2DDataObject() override;

    /** Not supported by this class as the parameters are not valid with the ctor. */
    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override;

    /**
     * @return whether valid scalar data was found in the constructor. Otherwise, just delete your
     * instance..
     */
    bool isValid() const;

    bool is3D() const override;
    IndexType defaultAttributeLocation() const override;
    std::unique_ptr<Context2DData> createContextData() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    const QString & abscissa() const;

    const QString & scalarsName() const;
    IndexType scalarsLocation() const;
    vtkIdType vectorComponent() const;

    ValueRange<> scalarRange();
    int numberOfScalars();

    /** @return X,Y-coordinates for the first point */
    const vtkVector2d & profileLinePoint1() const;
    /** @return X,Y-coordinates for the second point */
    const vtkVector2d & profileLinePoint2() const;
    void setProfileLinePoints(const vtkVector2d & point1, const vtkVector2d & point2);
    /**
     * Convenience method to request the points to be transformed from their current coordinate
     * system to the coordinate system of the source data set.
     * If the specified coordinate system is empty, not valid, or not supported, the points won't
     * be transformed and used as is.
     */
    void setPointsCoordinateSystem(const CoordinateSystemSpecification & coordsSpec);
    const CoordinateSystemSpecification & pointsCoordinateSystem() const;

signals:
    /**
     * Emitted when the source data values are modified.
     * This does not necessarily mean that the profile values change, but updating might be useful.
     * This is only emitted as long as the source DataObject instance exists.
     */
    void sourceDataChanged();

protected:
    vtkAlgorithmOutput * processedOutputPortInternal() override;
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    void updateLinePointsTransform();
    void updateLinePoints();

private:
    bool m_isValid;
    QString m_abscissa;
    QString m_scalarsName;
    IndexType m_scalarsLocation;
    vtkIdType m_vectorComponent;

    vtkSmartPointer<vtkAlgorithm> m_sourceAlgorithm;

    vtkVector2d m_profileLinePoint1;
    vtkVector2d m_profileLinePoint2;
    CoordinateSystemSpecification m_profileLinePointsCoordsSpec;
    ReferencedCoordinateSystemSpecification m_targetCoordsSpec;
    bool m_doTransformPoints;
    GeographicTransformationUtil m_pointsTransform;

    // extraction from vtkImageData
    bool m_inputIsImage;
    vtkSmartPointer<vtkLineSource> m_probeLine;

    // extraction from vtkPolyData
    vtkSmartPointer<LineOnCellsSelector2D> m_polyCentroidsSelector;
    vtkSmartPointer<LineOnPointsSelector2D> m_polyPointsSelector;

    vtkSmartPointer<vtkTransformPolyDataFilter> m_outputTransformation;
    vtkSmartPointer<vtkWarpScalar> m_graphLine;

private:
    Q_DISABLE_COPY(DataProfile2DDataObject)
};
