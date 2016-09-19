#pragma once

#include <cassert>
#include <memory>

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
    AbstractVisualizedData_private(AbstractVisualizedData & q_ptr,
        ContentType contentType, DataObject & dataObject)
        : contentType{ contentType }
        , dataObject{ dataObject }
        , isVisible{ true }
        , colorMappingData{ nullptr }
        , gradient{}
        , colorMapping{}
        , q_ptr{ q_ptr }
        , m_visibleBounds{}
        , m_visibleBoundsAreValid{ false }
    {
    }

    const ContentType contentType;
    DataObject & dataObject;
    bool isVisible;

    ColorMappingData * colorMappingData;
    vtkSmartPointer<vtkScalarsToColors> gradient;

    /** Color mappings can be shared between multiple visualizations. */
    std::unique_ptr<ColorMapping> colorMapping;

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
    AbstractVisualizedData & q_ptr;

    DataBounds m_visibleBounds;
    bool m_visibleBoundsAreValid;

private:
    AbstractVisualizedData_private(const AbstractVisualizedData_private &) = delete;
    void operator=(const AbstractVisualizedData_private &) = delete;
};


vtkInformationKeyMacro(AbstractVisualizedData_private, VisualizedDataKey, IntegerPointer);
