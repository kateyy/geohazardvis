#pragma once

#include <memory>
#include <utility>

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>
#include <core/utility/DataExtent_fwd.h>


class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkDataSet;
class vtkInformation;
class vtkScalarsToColors;
namespace reflectionzeug
{
    class PropertyGroup;
}

class AbstractVisualizedData_private;
class ColorMapping;
enum class ContentType;
class DataObject;
class ColorMappingData;


/**
 * Base class for visual representations of data objects.
 *
 * A data object may be visualized in multiple views, each holding its own AbstractVisualizedData
 * instance, referring to the data object.
 * This base class does not define how the visualization is implemented, it only defines the following
 * common features:
 *     - Color Mapping:
 *         Each visualization own a ColorMapping that provides ColorMappingData subclasses appropriate
 *         for this visualization. At least a default (no-op) mapping is always available.
 *     - Output Ports:
 *         Visualization may provide multiple output ports for different parts/kinds of visual objects.
 *     - Information Reference:
 *         Visualization reference themselves in their visualization pipeline using the
 *         setupInformation function. This allows to quickly link from a visible object, e.g., after
 *         picking, back to the related AbstractVisualizedData instance.
*/
class CORE_API AbstractVisualizedData : public QObject
{
    Q_OBJECT

public:
    AbstractVisualizedData(ContentType contentType, DataObject & dataObject);
    ~AbstractVisualizedData() override;

    ContentType contentType() const;

    DataObject & dataObject();
    const DataObject & dataObject() const;

    bool isVisible() const;
    void setVisible(bool visible);

    /** @return spatial bounds of the data sets used for the current visualization */
    const DataBounds & visibleBounds();

    virtual std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() = 0;

    /*
     * Color mapping used for this visualization. If it doesn't current has a color mapping, a new
     * one will be created.
     */
    ColorMapping & colorMapping();

    /**
     * This is used by the ColorMapping to set the current scalars. Don't call this from anywhere
     * else. */
    void setScalarsForColorMapping(ColorMappingData * scalars);
    /**
     * Set gradient that will be applied to colored geometries.
     * ColorMappingData are responsible for gradient configuration.
     */
    void setColorMappingGradient(vtkScalarsToColors * gradient);

    virtual unsigned int numberOfOutputPorts() const;
    vtkAlgorithmOutput * processedOutputPort(unsigned int port = 0);
    vtkDataSet * processedOutputDataSet(unsigned int port = 0);

    /**
     * Allow to inject additional post processing steps to the visualization pipeline without
     * directly modifying the AbstractVisualizedData class hierarchy.
     */
    struct PostProcessingStep
    {
        /** Visualization output port that this filter is applied to. */
        unsigned int visualizationPort;
        /** Processing step entry point that is connected to the current endpoint of the pipeline. */
        vtkSmartPointer<vtkAlgorithm> pipelineHead;
        /** Last step of processing step that will be the new endpoint of the pipeline. */
        vtkSmartPointer<vtkAlgorithm> pipelineTail;
    };
    /**
     * Add a post processing step to the visualization pipeline.
     * @return A boolean that is true if the step was successfully added, and an ID that can
     *  later be used to erase the processing step from the pipeline.
     */
    std::pair<bool, unsigned int> injectPostProcessingStep(const PostProcessingStep & postProcessingStep);
    bool erasePostProcessingStep(unsigned int id);

    /**
     * Identifier for a registered kind of post processing steps.
     * This cookie is identified by its id, not by its instance. Thus, it can be copied as needed.
     */
    struct StaticProcessingStepCookie
    {
        bool operator<(StaticProcessingStepCookie rhs) const { return id < rhs.id; }
        const int id;
    };
    /**
     * Statically register a specific type of processing steps.
     * Typically, a user would statically request a cookie for each kind of processing step that
     * is planned to be injected in visualization pipelines. Later on, for an instance of 
     * AbstractVisualizedData and a specific visualization port in it, this cookie is used to
     * check if a specific processing step is already present, and it can be injected or removed
     * from the pipeline (per port!).
     * That way it is not required for users to track for each visualizations its processing step
     * instances.
     */
    static StaticProcessingStepCookie requestProcessingStepCookie();
    /**
     * Inject a specific kind of post processing step, identified by the cookie and unique per for
     * each pair of visualization instance and visualization port.
     * Statically registered post processing step types are injected before other (dynamic) types.
     * The statically registered types are injected into the pipeline in the order their cookies
     * were requested. (This is only meaningful in local scope, not across compilation units etc.)
     * @return true only if the postProcessingStep is valid and the cookie is not used yet for
     * this visualization instance.
     */
    bool injectPostProcessingStep(
        const StaticProcessingStepCookie & cookie,
        const PostProcessingStep & postProcessingStep);
    bool hasPostProcessingStep(const StaticProcessingStepCookie & cookie, unsigned int port) const;
    /**
     * Retrieve a uniquely identified post processing step.
     * @return nullptr if the post processing step was not injected before. Returns a valid step
     *  otherwise.
     */
    PostProcessingStep * getPostProcessingStep(const StaticProcessingStepCookie & cookie, unsigned int port);
    bool erasePostProcessingStep(const StaticProcessingStepCookie & cookie, unsigned int port);

    /** store data object name and pointers in the information object */
    static void setupInformation(vtkInformation & information, AbstractVisualizedData & visualization);

    static AbstractVisualizedData * readPointer(vtkInformation & information);
    static void storePointer(vtkInformation & information, AbstractVisualizedData * visualization);

signals:
    void visibilityChanged(bool visible);
    /** Emitted when the underlaying data or its visible representation was modified */
    void geometryChanged();

protected:
    friend class AbstractVisualizedData_private;
    explicit AbstractVisualizedData(std::unique_ptr<AbstractVisualizedData_private> d_ptr);
    AbstractVisualizedData_private & dPtr();
    const AbstractVisualizedData_private & dPtr() const;

    /**
     * Allows subclasses to statically customize the visualization pipeline.
     * Default output is dataObject().processedOutputPort().
     */
    virtual vtkAlgorithmOutput * processedOutputPortInternal(unsigned int port);

    /** Subclasses can override this method to set initial parameters on a newly created color mapping */
    virtual void setupColorMapping(ColorMapping & colorMapping);
    virtual void visibilityChangedEvent(bool visible);
    virtual void scalarsForColorMappingChangedEvent();
    virtual void colorMappingGradientChangedEvent();

    /** Convenience method to return the current data mapped to colors. This may be nullptr */
    ColorMappingData * currentColorMappingData();
    vtkScalarsToColors * currentColorMappingGradient();

    /** Called by sub-classes to update what is returned by visibleBounds() */
    virtual DataBounds updateVisibleBounds() = 0;
    /**
     * Called by sub-classes when then visible bounds changed.
     * updateVisibleBounds() will be called again once visibleBounds() is called the next time.
     */
    void invalidateVisibleBounds();

private:
    std::unique_ptr<AbstractVisualizedData_private> d_ptr;

private:
    Q_DISABLE_COPY(AbstractVisualizedData)
};
