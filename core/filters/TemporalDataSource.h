#pragma once

#include <array>
#include <vector>

#include <vtkDataSetAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>

#include <core/core_api.h>


/** Enriches pipeline data with attributes per time step.
  *
  * This filter adds point or cell attributes associated with time steps to its upstream data.
  * Besides attributes defined in this filter, the upstream vtkDataSet is passed through to the
  * output.
  * Temporal information will only be passed downstream if:
  *     - Temporal attributes are defined
  *     - Time steps are requested by the downstream
  *     - Requested time steps exactly match with available time steps.
  * Multiple attribute with different time steps can be defined in this filter, but it is up to the
  * downstream to handle such cases sensibly. The time steps and range passed in the information
  * update step always refer to the whole, sorted list of available time steps. */
class CORE_API TemporalDataSource : public vtkDataSetAlgorithm
{
public:
    vtkTypeMacro(TemporalDataSource, vtkDataSetAlgorithm);
    static TemporalDataSource * New();

    enum AttributeLocation : unsigned int
    {
        POINT_DATA = 0u,
        CELL_DATA = 1u,
        NUM_VALUES
    };

    /** Add a temporal attribute uniquely identified by its location and name.
      * If this attribute already exists, it will not be overwritten but reused instead.
      * Use SetTemporalAttributeTimeStep to add values for specific time steps of the attribute.
      * @return index that is used to quickly lookup the attribute. Returns -1 if the attribute
      *     could not be added (too many attributes at the selected location). */
    int AddTemporalAttribute(AttributeLocation attributeLoc, const vtkStdString & name);
    /** Check if a specific attribute already exists.
      * @return index of the selected attribute, or -1 if it does not exist. */
    int TemporalAttributeIndex(AttributeLocation attributeLoc, const vtkStdString & name);
    /** Remove a temporal attribute by name or index.
      * @return whether the attribute existed before this call. */
    bool RemoveTemporalAttribute(AttributeLocation attributeLoc, const vtkStdString & name);
    bool RemoveTemporalAttribute(AttributeLocation attributeLoc, int temporalAttributeIndex);
    /** Set the data for time step of a temporal attribute that was previously added using AddTemporalAttribute.
     * This sets the vtkDataObject::DATA_TIME_STEP to timeStep, if array is not nullptr.
     * This function replaces data that was previously defined for the same time step, attribute
     * and location.
     * @return false if the temporalAttributeIndex is out of range. */
    bool SetTemporalAttributeTimeStep(AttributeLocation attributeLoc,
        int temporalAttributeIndex,
        double timeStep,
        vtkAbstractArray * array);
    /** This is equivalent to the previous function, but requires vtkDataObject::DATA_TIME_STEP
    * to be set on the passed array. Also, array must not be nullptr. */
    bool SetTemporalAttributeTimeStep(AttributeLocation attributeLoc,
        int temporalAttributeIndex,
        vtkAbstractArray * array);

protected:
    TemporalDataSource();
    ~TemporalDataSource() override;

    int ProcessRequest(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestInformation(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestUpdateExtent(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

    int RequestData(vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    struct AttributeAtTimeStep
    {
        double TimeStep;
        vtkSmartPointer<vtkAbstractArray> Attribute;
        bool operator<(const AttributeAtTimeStep & other) const;
        bool operator<(double other) const;
    };
    friend bool operator<(double lhs, const TemporalDataSource::AttributeAtTimeStep & rhs);
    struct TemporalAttribute
    {
        vtkStdString Name;
        std::vector<AttributeAtTimeStep> Data;
    };

    std::array<std::vector<TemporalAttribute>, static_cast<size_t>(AttributeLocation::NUM_VALUES)> TemporalData;
    decltype(TemporalData)::value_type & temporalData(AttributeLocation attributeLoc);

private:
    TemporalDataSource(const TemporalDataSource &) = delete;
    void operator=(const TemporalDataSource &) = delete;
};
