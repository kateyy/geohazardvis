#pragma once

#include <QMap>

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/DataObject.h>


class vtkAlgorithm;


/** Superclass for data objects that can be represented in different coordinate systems.

This class adds interfaces to request differently transformed coordinate data (geographic global,
metric global or local).
*/
class CORE_API CoordinateTransformableDataObject : public DataObject
{
    Q_OBJECT

public:
    /** Construct a DataObject and read coordinate system information from dataSet, if available.
    @param dataSet If this is set to nullptr, coordinate system specifications will not be
        initialized and need to be set using specifyCoordinateSystem() in order to allow for
        transformations.
    */
    explicit CoordinateTransformableDataObject(const QString & name, vtkDataSet * dataSet);
    ~CoordinateTransformableDataObject() override;

    const ReferencedCoordinateSystemSpecification & coordinateSystem() const;

    /** Check whether enough information is available to transform the data object into other
    coordinate systems.
    If this returns missingInformation, additional information has to requested from the user and
    supplied to specifyCoordinateSystem().
    If this returns unsupported, kindly inform the user of the inconvenience. */
    ConversionCheckResult canTransformTo(const CoordinateSystemSpecification & coordinateSystem);

    /** If stored information regarding the coordinate system of the data set is not set or wrong,
    it can be corrected with this function.
    This will not do any transformation, it's purely a modification of meta data.
    NOTE: This invalidates previous output ports and data sets!
    @return false, if the specified parameters are invalid.
    */
    bool specifyCoordinateSystem(const ReferencedCoordinateSystemSpecification & coordinateSystem);

    /** Request the dataset transformed to the specified coordinate system. */
    vtkAlgorithmOutput * coordinateTransformedOutputPort(const CoordinateSystemSpecification & spec);
    /** Convenience method returning the data set produced by the selected coordinate transformation.
    This data set becomes invalid onces specifyCoordinateSystem() is called.
    This may be nullptr, if the selected transformation is not supported. */
    vtkSmartPointer<vtkDataSet> coordinateTransformedDataSet(const CoordinateSystemSpecification & spec);

signals:
    /** Emitted when the data objects coordinate system was changed using specifyCoordinateSystem() */
    void coordinateSystemChanged();

protected:
    /** Setup the part of the pipeline that transform coordinates in one direction.
    This needs to be implemented by subclasses and is call by this superclass as required.
    This may return an empty pointer, in which case the transformation is considered as not supported.
    */
    virtual vtkSmartPointer<vtkAlgorithm> createTransformPipeline(
        const CoordinateSystemSpecification & toSystem,
        vtkAlgorithmOutput * pipelineUpstream) const;

private:
    ConversionCheckResult canTransformToInternal(const CoordinateSystemSpecification & coordinateSystem) const;

    /** Information object of the basic data set of this object.
    This data set is either provided in the constructor, or using processedOutputPort().
    This may also be nullptr, if subclasses do not provide a data set at all. In this case,
    coordinate system information will only be stored temporarily.
    */
    vtkInformation * informationStorage();

    /** Fall-back if transformation is not supported, and no-op, if no transformation is required */
    vtkAlgorithm * passThrough();

    vtkAlgorithm * transformFilter(const CoordinateSystemSpecification & toSystem);

private:
    ReferencedCoordinateSystemSpecification m_spec;

    vtkSmartPointer<vtkAlgorithm> m_passThrough;
    /** The pipeline output m_pipelines[type, geoName, metricName] converts the current data set to 
    a representation of in type, using the geographic coordinates geoName, and depending on the
    type, coordinates in the metricName system. */
    QMap<CoordinateSystemType, QMap<QString, QMap<QString, vtkSmartPointer<vtkAlgorithm>>>> m_pipelines;

private:
    Q_DISABLE_COPY(CoordinateTransformableDataObject);
};
