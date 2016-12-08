#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>
#include <utility>

#include <vtkInformation.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkSmartPointer.h>

#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/utility/DataExtent.h>


class vtkScalarsToColors;

class AbstractVisualizedData;
class ColorMappingData;
class DataObject;


class CORE_API AbstractVisualizedData_private
{
public:
    AbstractVisualizedData_private(ContentType contentType, DataObject & dataObject)
        : contentType{ contentType }
        , dataObject{ dataObject }
        , isVisible{ true }
        , colorMappingData{ nullptr }
        , gradient{}
        , colorMapping{}
        , m_nextProcessingStepId{ 0 }
        , m_visibleBounds{}
        , m_visibleBoundsAreValid{ false }
    {
    }

    ~AbstractVisualizedData_private() = default;

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
    void releaseProcessingStepId(unsigned int id)
    {
        m_freedProcessingStepIds.push_back(id);
    }
    std::vector<std::vector<std::pair<unsigned int, AbstractVisualizedData::PostProcessingStep>>> postProcessingStepsPerPort;
    std::vector<vtkSmartPointer<vtkAlgorithmOutput>> pipelineEndpointsPerPort;

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

    static vtkInformationIntegerPointerKey * VisualizedDataKey();

private:
    DataBounds m_visibleBounds;
    bool m_visibleBoundsAreValid;

    int m_nextProcessingStepId;
    std::vector<int> m_freedProcessingStepIds;

private:
    AbstractVisualizedData_private(const AbstractVisualizedData_private &) = delete;
    void operator=(const AbstractVisualizedData_private &) = delete;
};


vtkInformationKeyMacro(AbstractVisualizedData_private, VisualizedDataKey, IntegerPointer);
