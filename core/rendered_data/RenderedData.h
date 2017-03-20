#pragma once

#include <vtkSmartPointer.h>

#include <core/AbstractVisualizedData.h>


class vtkPropCollection;

struct CoordinateSystemSpecification;
class CoordinateTransformableDataObject;
class RenderedData_private;


/**
 * Base class for rendered representations of data objects.
 * 
 * Subclasses are visualized in render views that are based on vtkRenderer/vtkRenderWindow.
 * 
 * This class assumes that the source data object is representable in different coordinate systems
 * (CoordinateTransformableDataObject subclass). This allows to visualize multiple data objects
 * with different source coordinate systems in common coordinate system of the render view.
 * 
 * The following pipeline is set up, whereas "ConcreteData" refers to a subclass of
 * CoordinateTransformableDataObject.
 *   - Source data: Instance of vtkDataSet subclass.
 *   - ConcreteData::processedOutputPort: Allow specific enhancements, e.g., computing normals.
 *   - transformedCoordinatesOutputPort: Integrates ConcreteData::coordinateTransformedOutputPort,
 *     or vtkPassThrough into the pipeline to output the data set transformed to what was specified by
 *     setDefaultCoordinateSystem().
 *   - Contents of viewProps() should use transformedCoordinatesOutputPort() as upstream, so that all
 *     visualizations in a view use the same coordinate system.
*/
class CORE_API RenderedData : public AbstractVisualizedData
{
    Q_OBJECT

public:
    explicit RenderedData(ContentType contentType, CoordinateTransformableDataObject & dataObject);
    ~RenderedData() override;

    CoordinateTransformableDataObject & transformableObject();
    const CoordinateTransformableDataObject & transformableObject() const;

    /**
     * Set a default coordinate system that is used in the current render view.
     * Initially, this is set to geographic, "UTM-WGS84".
     * This function defines which output will be produced by transformedCoordinatesOutputPort()
     * and transformedCoordinatesDataSet().
    */
    void setDefaultCoordinateSystem(const CoordinateSystemSpecification & coordinateSystem);
    const CoordinateSystemSpecification & defaultCoordinateSystem() const;

    /**
     * @return the source data transformed into the coordinate system specified by
     * setDefaultCoordinateSystem().
     * If the source data cannot be transformed into the requested system (see
     * CoordinateTransformableDataObject::canTransformTo()), this will just pass through the data.
     */
    vtkAlgorithmOutput * transformedCoordinatesOutputPort();
    /**
     * Convenience method that returns a persistent shallow copy of the output data set of
     * transformedCoordinatesOutputPort().
     */
    vtkDataSet * transformedCoordinatesDataSet();


    enum class Representation { content, outline, both };

    Representation representation() const;
    void setRepresentation(Representation representation);

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    /** VTK view props visualizing the data set object and possibly additional attributes */
    vtkSmartPointer<vtkPropCollection> viewProps();

signals:
    void viewPropCollectionChanged();

protected:
    vtkAlgorithmOutput * processedOutputPortInternal(unsigned int port) override;
    void visibilityChangedEvent(bool visible) override;
    virtual void representationChangedEvent(Representation representation);
    virtual vtkSmartPointer<vtkPropCollection> fetchViewProps() = 0;
    void invalidateViewProps();

private:
    RenderedData_private & dPtr();
    const RenderedData_private & dPtr() const;

private:
    Q_DISABLE_COPY(RenderedData)
};
