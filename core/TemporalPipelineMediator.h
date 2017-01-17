#pragma once

#include <vector>

#include <QObject>
#include <QString>

#include <vtkType.h>

#include <core/core_api.h>


class AbstractVisualizedData;


class CORE_API TemporalPipelineMediator : public QObject
{
public:
    TemporalPipelineMediator();
    ~TemporalPipelineMediator() override;

    void setVisualization(AbstractVisualizedData * visualization);
    AbstractVisualizedData * visualzation();
    const AbstractVisualizedData * visualzation() const;

    /** @return Sorted list of time steps that the current visualization contains.
      * This is empty if no visualization is set, or the visualization doesn't contain temporal data. */
    const std::vector<double> & timeSteps() const;

    /** Select a specific time step and instruct the visualization to report temporal attributes
      * for this specific time step.*/
    void selectTimeStepByIndex(size_t index);
    /** Index of the currently selected time step. This value is undefined when interpolating
      * between time steps. */
    size_t currentTimeStepIndex() const;
    double selectedTimeStep() const;

private:
    bool updateTimeSteps();
    bool selectionFromPipeline();
    void passSelectionToPipeline();

private:
    AbstractVisualizedData * m_visualization;
    std::vector<double> m_timeSteps;

    vtkMTimeType m_pipelineModifiedTime;

    struct TimeStepSelection
    {
        TimeStepSelection();

        size_t index;
        double selectedTimeStep;
    };
    TimeStepSelection m_selection;

private:
    Q_DISABLE_COPY(TemporalPipelineMediator)
};
