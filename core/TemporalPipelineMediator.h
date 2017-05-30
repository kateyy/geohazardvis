#pragma once

#include <utility>
#include <vector>

#include <QObject>
#include <QString>

#include <vtkType.h>

#include <core/core_api.h>


class vtkAlgorithm;
template<typename T> class vtkSmartPointer;
class AbstractVisualizedData;


class CORE_API TemporalPipelineMediator : public QObject
{
public:
    using TimeStep_t = double;

    TemporalPipelineMediator();
    ~TemporalPipelineMediator() override;

    void setVisualization(AbstractVisualizedData * visualization);
    /*
     * The temporal selection handled by the mediator is stored within the visualization, even
     * if the mediator itself does not currently reference the visualization. Use this function
     * to undo any settings done by the mediator on the visualization.
     * If the mediator is currently set to the visualization, its current visualization will be
     * nullptr after this call.
     */
    void unregisterFromVisualization(AbstractVisualizedData * visualization);
    AbstractVisualizedData * visualization();
    const AbstractVisualizedData * visualization() const;

    /**
     * @return Sorted list of time steps that the current visualization contains.
     * This is empty if no visualization is set, or the visualization doesn't contain temporal data.
     */
    const std::vector<TimeStep_t> & timeSteps() const;

    /**
     * Select a specific time step and instruct the visualization to report temporal attributes
     * for this specific time step.
     */
    void selectTimeStepByIndex(size_t index);
    /**
     * Index of the currently selected time step. This value is undefined when interpolating
     * between time steps.
     */
    size_t currentTimeStepIndex() const;
    TimeStep_t selectedTimeStep() const;

    /**
     * Visualize the difference between two time steps.
     * For all attributes that vary over time, values at beginTimeStep are subtracted from values
     * at endTimeStep. endTimeStep can be "before" beginTimeStep, which inverts the difference.
     */
    void selectTemporalDifferenceByIndex(size_t beginIndex, size_t endIndex);
    std::pair<TimeStep_t, TimeStep_t> differenceTimeSteps() const;
    std::pair<size_t, size_t> differenceTimeStepIndices() const;

    /** Helper that stores temporal selection configuration and may apply it to a VTK pipeline. */
    class CORE_API TemporalSelection
    {
    public:
        bool isValid;
        bool isTimeRange;
        TimeStep_t beginTimeStep;
        TimeStep_t endTimeStep;

        /**
         * Create an algorithm that applies the temporal selection to a VTK pipeline.
         * @return nullptr, if isValid is false. Otherwise, this creates a valid algorithm, call
         *  configureAlgorithm() on it and returns it.
         */
        vtkSmartPointer<vtkAlgorithm> createAlgorithm() const;
        /**
         * Apply the parameters to the supplied algorithm, if *this is valid, and the supplied
         * algorithm is valid for the current value of isTimeRange.
         */
        void configureAlgorithm(vtkAlgorithm & algorithm,
            bool * isValidAlgorithm = nullptr, bool * modified = nullptr) const;

    private:
        void configureAlgorithmInternal(vtkAlgorithm & algorithm, bool * modified) const;
    };
    static TemporalSelection currentPipelineSelection(AbstractVisualizedData & visualization, unsigned int port = 0);

private:
    bool updateTimeSteps();
    bool selectionFromPipeline();
    void passSelectionToPipeline();

private:
    AbstractVisualizedData * m_visualization;
    std::vector<TimeStep_t> m_timeSteps;

    vtkMTimeType m_pipelineModifiedTime;

    struct SelectionInternal
    {
        SelectionInternal();
        SelectionInternal(bool isTimeRange,
            size_t beginIndex, TimeStep_t beginTimeStep,
            size_t endIndex, TimeStep_t endTimeStep);

        bool isTimeRange;
        size_t beginIndex;
        TimeStep_t beginTimeStep;
        size_t endIndex;
        TimeStep_t endTimeStep;

        void setTimePoint(size_t index, TimeStep_t timeStep);
        bool equalsTimePoint(size_t index, TimeStep_t timeStep) const;

        bool operator==(const SelectionInternal & other) const;
        bool operator!=(const SelectionInternal & other) const;

        TemporalSelection toTemporalSelection() const;
    };
    SelectionInternal m_selection;

private:
    Q_DISABLE_COPY(TemporalPipelineMediator)
};
