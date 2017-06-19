#pragma once

#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include <vtkInformation.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkSmartPointer.h>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/utility/DataExtent.h>


class vtkPassThrough;
class vtkScalarsToColors;

class AbstractVisualizedData;
class ColorMappingData;
class DataObject;


class CORE_API AbstractVisualizedData_private
{
public:
    AbstractVisualizedData_private(AbstractVisualizedData & q_ptr,
        ContentType contentType, DataObject & dataObject)
        : q_ptr{ q_ptr }
        , contentType{ contentType }
        , dataObject{ dataObject }
        , isVisible{ true }
        , colorMappingData{ nullptr }
        , gradient{}
        , colorMapping{}
        , m_visibleBounds{}
        , m_visibleBoundsAreValid{ false }
        , m_nextProcessingStepId{ 0 }
    {
    }

    virtual ~AbstractVisualizedData_private() = default;

    AbstractVisualizedData & q_ptr;

    const ContentType contentType;
    DataObject & dataObject;
    bool isVisible;

    ColorMappingData * colorMappingData;
    vtkSmartPointer<vtkScalarsToColors> gradient;

    /** Color mappings can be shared between multiple visualizations. */
    std::unique_ptr<ColorMapping> colorMapping;

    unsigned int getNextProcessingStepId()
    {
        if (!m_freedProcessingStepIds.empty())
        {
            const auto id = m_freedProcessingStepIds.back();
            m_freedProcessingStepIds.pop_back();
            return id;
        }

        return m_nextProcessingStepId++;
    }
    void releaseProcessingStepId(const unsigned int id)
    {
        m_freedProcessingStepIds.push_back(id);
    }
    using DynamicPPStepType = std::pair<unsigned int, AbstractVisualizedData::PostProcessingStep>;
    struct PostProcessingSteps
    {
        /** Using unique_ptr for the simple structs to be able to hand out thread safe pointers
         * to a specific post processing step. Thread safe as long the step itself is not modified
         * or removed from the pipeline. */
        std::map<AbstractVisualizedData::StaticProcessingStepCookie,
            std::unique_ptr<AbstractVisualizedData::PostProcessingStep>> staticStepTypes;
        std::vector<DynamicPPStepType> dynamicStepTypes;
    };

// There seems to be a bug in VS 2017's std::vector not consistently using move semantics.
#if defined(_MSC_VER) && _MSC_VER >= 1910 && _MSC_FULL_VER <= 191025019
    template<typename T>
    class IndexableList : public std::list<T>
    {
    public:
        using std::list<T>::list;

        T & operator[](const size_t index)
        {
            assert(this->size() > index);
            return *std::next(begin(), index);
        }
    };

    using PostProcessingStepsVector = IndexableList<PostProcessingSteps>;
#else
    using PostProcessingStepsVector = std::vector<PostProcessingSteps>;
#endif
    PostProcessingStepsVector postProcessingStepsPerPort;

    /**
     * Persistent processing pipeline end points.
     * These vtkPassThrough's make sure that downstream algorithms can reliably connect to the
     * visualization, no matter when the post processing pipeline is modified by other consumers.
     * Thus, the output of the function must never be a post processing step that might get deleted
     * at some point.
     */
    vtkAlgorithm * pipelineEndpointsAtPort(const unsigned int port);
    void updatePipeline(const unsigned int port);

    const DataBounds & visibleBounds() const
    {
        assert(m_visibleBoundsAreValid);
        return m_visibleBounds;
    }

    void validateVisibleBounds(const DataBounds & visibleBounds)
    {
        m_visibleBounds = visibleBounds;
        m_visibleBoundsAreValid = true;
    }

    void invalidateVisibleBounds()
    {
        m_visibleBoundsAreValid = false;
    }

    bool visibleBoundsAreValid() const
    {
        return m_visibleBoundsAreValid;
    }

    static vtkInformationIntegerPointerKey * VISUALIZED_DATA();

private:
    DataBounds m_visibleBounds;
    bool m_visibleBoundsAreValid;

    unsigned int m_nextProcessingStepId;
    std::vector<unsigned int> m_freedProcessingStepIds;
    std::vector<vtkSmartPointer<vtkPassThrough>> m_pipelineEndpointsAtPort;

private:
    AbstractVisualizedData_private(const AbstractVisualizedData_private &) = delete;
    void operator=(const AbstractVisualizedData_private &) = delete;
};
