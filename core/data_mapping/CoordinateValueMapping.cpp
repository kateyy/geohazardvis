#include "CoordinateValueMapping.h"

#include <limits>

#include <vtkElevationFilter.h>

#include <core/Input.h>
#include <core/data_objects/PolyDataObject.h>

#include "ScalarsForColorMappingRegistry.h"

const QString CoordinateXValueMapping::s_name = "x values";
const bool CoordinateXValueMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CoordinateXValueMapping>);
const QString CoordinateYValueMapping::s_name = "y values";
const bool CoordinateYValueMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CoordinateYValueMapping>);
const QString CoordinateZValueMapping::s_name = "z values";
const bool CoordinateZValueMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstance<CoordinateZValueMapping>);


AbstractCoordinateValueMapping::AbstractCoordinateValueMapping(const QList<DataObject *> & dataObjects)
    : ScalarsForColorMapping(dataObjects)
{
    for (DataObject * dataObject : dataObjects)
    {
        PolyDataObject * polyDataObject = dynamic_cast<PolyDataObject*>(dataObject);

        if (!polyDataObject)
            break;

        m_dataObjects << polyDataObject;
    }

    // applicable for list of PolyDataObjects only
    if (!m_dataObjects.length() == dataObjects.length())
        m_dataObjects.clear();
}

AbstractCoordinateValueMapping::~AbstractCoordinateValueMapping() = default;

bool AbstractCoordinateValueMapping::usesGradients() const
{
    return true;
}

vtkAlgorithm * AbstractCoordinateValueMapping::createFilter()
{
    vtkWeakPointer<vtkElevationFilter> elevation = vtkElevationFilter::New();

    m_filters << elevation;

    minMaxChanged();    // trigger elevation update

    return elevation;
}

bool AbstractCoordinateValueMapping::isValid() const
{
    return !m_dataObjects.isEmpty();
}

void AbstractCoordinateValueMapping::minMaxChangedEvent()
{
    // remove empty weak pointers

    auto it = m_filters.begin();
    while (it != m_filters.end())
    {
        if (*it == nullptr)
            it = m_filters.erase(it);
        else
            ++it;
    }

    ScalarsForColorMapping::minMaxChangedEvent();
}

CoordinateXValueMapping::CoordinateXValueMapping(const QList<DataObject *> & dataObjects)
    : AbstractCoordinateValueMapping(dataObjects)
{
}

QString CoordinateXValueMapping::name() const
{
    return s_name;
}

void CoordinateXValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_dataMinValue = std::min(m_dataMinValue, dataObject->input()->bounds()[0]);
        m_dataMaxValue = std::max(m_dataMaxValue, dataObject->input()->bounds()[1]);
    }
    ScalarsForColorMapping::updateBounds();
}

void CoordinateXValueMapping::minMaxChangedEvent()
{
    AbstractCoordinateValueMapping::minMaxChangedEvent();

    // prevent degenerated vector
    double fixedMax = m_maxValue;
    if (m_minValue == m_maxValue)
        fixedMax += std::numeric_limits<double>::epsilon();

    for (vtkElevationFilter * elevation : m_filters)
    {
        elevation->SetLowPoint(m_minValue, 0, 0);
        elevation->SetHighPoint(fixedMax, 0, 0);
    }
}


CoordinateYValueMapping::CoordinateYValueMapping(const QList<DataObject *> & dataObjects)
    : AbstractCoordinateValueMapping(dataObjects)
{
}

QString CoordinateYValueMapping::name() const
{
    return s_name;
}

void CoordinateYValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_dataMinValue = std::min(m_dataMinValue, dataObject->input()->bounds()[2]);
        m_dataMaxValue = std::max(m_dataMaxValue, dataObject->input()->bounds()[3]);
    }
    ScalarsForColorMapping::updateBounds();
}

void CoordinateYValueMapping::minMaxChangedEvent()
{
    AbstractCoordinateValueMapping::minMaxChangedEvent();

    // prevent degenerated vector
    double fixedMax = m_maxValue;
    if (m_minValue == m_maxValue)
        fixedMax += std::numeric_limits<double>::epsilon();

    for (vtkElevationFilter * elevation : m_filters)
    {
        elevation->SetLowPoint(0, m_minValue, 0);
        elevation->SetHighPoint(0, fixedMax, 0);
    }
}


CoordinateZValueMapping::CoordinateZValueMapping(const QList<DataObject *> & dataObjects)
    : AbstractCoordinateValueMapping(dataObjects)
{
}

QString CoordinateZValueMapping::name() const
{
    return s_name;
}

void CoordinateZValueMapping::updateBounds()
{
    for (PolyDataObject * dataObject : m_dataObjects)
    {
        m_dataMinValue = std::min(m_dataMinValue, dataObject->input()->bounds()[4]);
        m_dataMaxValue = std::max(m_dataMaxValue, dataObject->input()->bounds()[5]);
    }
    ScalarsForColorMapping::updateBounds();
}

void CoordinateZValueMapping::minMaxChangedEvent()
{
    AbstractCoordinateValueMapping::minMaxChangedEvent();

    // prevent degenerated vector
    double fixedMax = m_maxValue;
    if (m_minValue == m_maxValue)
        fixedMax += std::numeric_limits<double>::epsilon();

    for (vtkElevationFilter * elevation : m_filters)
    {
        elevation->SetLowPoint(0, 0, m_minValue);
        elevation->SetHighPoint(0, 0, fixedMax);
    }
}
