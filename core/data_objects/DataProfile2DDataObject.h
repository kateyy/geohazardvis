#pragma once

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/DataExtent_fwd.h>


class vtkAlgorithm;
class vtkLineSource;
class vtkTransformPolyDataFilter;
class vtkWarpScalar;

enum class IndexType;
class CoordinateTransformableDataObject;
class LineOnCellsSelector2D;
class LineOnPointsSelector2D;
class SetCoordinateSystemInformationFilter;
class SimplePolyGeoCoordinateTransformFilter;


/**
* Probes the source data along a line defined by two points on the XY-plane and creates a plot for the interpolated data.
*/
class CORE_API DataProfile2DDataObject : public DataObject
{
public:
    struct PreprocessingPipeline
    {
        vtkSmartPointer<vtkAlgorithm> head;
        vtkSmartPointer<vtkAlgorithm> tail;
    };

    /** Create profile with given specifications
     * @param name Object Name, see DataObject API
     * @param sourceData Source data object that will be probed. The DataObject will only be used
        in the constructor, only shared VTK data sets are referenced later.
     * @param scalarsName Scalars to probe in the source data object 
     * @param scalarsLocation Specifies whether to probe point or cell scalars 
     * @param vectorComponent For multi component scalars/vectors, specify which component will be
        extracted
     * @param preprocessingPipeline Optional preprocessing pipeline. Will be injected between the
        processed/transformed output of the sourceData and the profile transformations. */
    DataProfile2DDataObject(
        const QString & name,
        DataObject & sourceData,
        const QString & scalarsName,
        IndexType scalarsLocation,
        vtkIdType vectorComponent,
        const PreprocessingPipeline & preprocessingPipeline = {});
    ~DataProfile2DDataObject() override;

    /** @return whether valid scalar data was found in the constructor. Otherwise, just delete your instance.. */
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
    /** Convenience method to request the points to be transformed from their current coordinate 
      * system to the coordinate system of the source data set.
      * If the specified coordinate system is empty, not valid, or not supported, the points won't
      * be transformed and used as is. */
    void setPointsCoordinateSystem(const CoordinateSystemSpecification & coordsSpec);
    const CoordinateSystemSpecification & pointsCoordinateSystem() const;

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
    vtkSmartPointer<SetCoordinateSystemInformationFilter> m_pointsSetCoordsSpecFilter;
    vtkSmartPointer<SimplePolyGeoCoordinateTransformFilter> m_pointsTransformFilter;

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
