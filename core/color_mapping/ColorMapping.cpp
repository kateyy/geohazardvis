#include "ColorMapping.h"

#include <cassert>

#include <QDebug>

#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/color_mapping/DefaultColorMapping.h>
#include <core/color_mapping/GlyphColorMappingGlyphListener.h>
#include <core/color_mapping/GradientResourceManager.h>


ColorMapping::ColorMapping()
    : QObject()
    , m_isEnabled{ false }
    , m_glyphListener{ std::make_unique<GlyphColorMappingGlyphListener>() }
{
    connect(m_glyphListener.get(), &GlyphColorMappingGlyphListener::glyphMappingChanged,
        this, &ColorMapping::updateAvailableScalars);
}

ColorMapping::~ColorMapping() = default;

void ColorMapping::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
    {
        return;
    }

    updateCurrentMappingState(m_currentScalarsName, enabled);
}

bool ColorMapping::isEnabled() const
{
    return m_isEnabled;
}

void ColorMapping::setVisualizedData(const QList<AbstractVisualizedData *> & visualizedData)
{
    // clean up old scalars
    for (auto vis : m_visualizedData)
    {
        vis->setScalarsForColorMapping(nullptr);
        disconnect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    auto lastScalars = currentScalarsName();
    m_currentScalarsName.clear();
    releaseMappingData();


    m_visualizedData = visualizedData;

    for (auto vis : m_visualizedData)
    {
        // pass our (persistent) gradient object
        vis->setColorMappingGradient(gradient());

        connect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    if (!m_visualizedData.isEmpty())
    {
        m_data = ColorMappingRegistry::instance().createMappingsValidFor(m_visualizedData);

        for (auto & pair : m_data)
        {
            auto & scalars = pair.second;
            scalars->setLookupTable(gradient());
        }
    }

    // Try to reuse previous configuration.
    // Re-enable color mapping only if the same scalars are still available to not confuse the user.
    QString newScalarsName = "";
    const bool lastScalarsAvailable = m_data.find(lastScalars) != m_data.end();
    if (lastScalarsAvailable)
    {
        newScalarsName = lastScalars;
    }
    else if (!m_data.empty())
    {
        newScalarsName = m_data.begin()->first;
    }

    updateCurrentMappingState(newScalarsName, m_isEnabled && lastScalarsAvailable);

    m_glyphListener->setData(visualizedData);
}

void ColorMapping::registerVisualizedData(AbstractVisualizedData * visualizedData)
{
    if (m_visualizedData.contains(visualizedData))
    {
        return;
    }

    auto newList = m_visualizedData;
    newList << visualizedData;

    setVisualizedData(newList);

    assert(m_visualizedData.contains(visualizedData));

    emit visualizedDataChanged();
}

void ColorMapping::unregisterVisualizedData(AbstractVisualizedData * visualizedData)
{
    if (!m_visualizedData.contains(visualizedData))
    {
        return;
    }

    auto newList = m_visualizedData;
    newList.removeOne(visualizedData);
    assert(!newList.contains(visualizedData));

    setVisualizedData(newList);

    assert(!m_visualizedData.contains(visualizedData));

    emit visualizedDataChanged();
}

bool ColorMapping::scalarsAvailable() const
{
    return !m_data.empty();
}

const QList<AbstractVisualizedData *> & ColorMapping::visualizedData() const
{
    return m_visualizedData;
}

QStringList ColorMapping::scalarsNames() const
{
    QStringList names;
    for (auto & s : m_data)
    {
        names << s.first;
    }

    return names;
}

const QString & ColorMapping::currentScalarsName() const
{
    return m_currentScalarsName;
}

void ColorMapping::setCurrentScalarsByName(const QString & scalarsName)
{
    setCurrentScalarsByName(scalarsName, m_isEnabled);
}

void ColorMapping::setCurrentScalarsByName(const QString & scalarsName, bool enableColorMapping)
{
    if (m_currentScalarsName == scalarsName && m_isEnabled == enableColorMapping)
    {
        return;
    }

    updateCurrentMappingState(scalarsName, enableColorMapping);
}

void ColorMapping::updateCurrentMappingState(const QString & scalarsName, bool enabled)
{
    auto & oldScalars = currentScalars();
    oldScalars.deactivate();

    // cleanup old mappings
    for (auto vis : m_visualizedData)
    {
        vis->setScalarsForColorMapping(nullptr);
    }

    m_currentScalarsName = scalarsName;
    m_isEnabled = enabled;

    auto & scalars = currentScalars();
    scalars.activate();

    emit currentScalarsChanged();
}

ColorMappingData & ColorMapping::nullColorMapping() const
{
    std::lock_guard<std::mutex> lock(m_nullMappingMutex);

    if (!m_nullColorMapping)
    {
        m_nullColorMapping = std::make_unique<DefaultColorMapping>(m_visualizedData);
    }

    return *m_nullColorMapping;
}

void ColorMapping::releaseMappingData()
{
    m_data.clear();
    m_nullColorMapping.reset();
}

ColorMappingData & ColorMapping::currentScalars()
{
    return const_cast<ColorMappingData &>(    // don't implement the same function twice
        (static_cast<const ColorMapping *>(this))->currentScalars());
}

const ColorMappingData & ColorMapping::currentScalars() const
{
    if (!m_isEnabled || currentScalarsName().isEmpty())
    {
        return nullColorMapping();
    }

    auto it = m_data.find(currentScalarsName());
    assert(it != m_data.end());
    if (it == m_data.end())
    {
        qDebug() << "Invalid scalars requested: " << currentScalarsName();
        return nullColorMapping();
    }

    return *it->second;
}

vtkLookupTable * ColorMapping::gradient()
{
    if (!m_gradient)
    {
        m_gradient = vtkSmartPointer<vtkLookupTable>::New();
    }

    if (m_gradientName.isEmpty())
    {
        setGradient(GradientResourceManager::instance().defaultGradientName());
    }

    return m_gradient;
}

vtkScalarsToColors * ColorMapping::scalarsToColors()
{
    return gradient();
}

const QString & ColorMapping::gradientName() const
{
    return m_gradientName;
}

void ColorMapping::setGradient(const QString & gradientName)
{
    if (m_gradientName == gradientName)
    {
        return;
    }

    m_gradientName = gradientName;

    auto & gradients = GradientResourceManager::instance().gradients();
    assert(gradients.find(gradientName) != gradients.end());

    auto && originalLut = gradients.at(gradientName).lookupTable;

    gradient()->SetTable(originalLut->GetTable());
    gradient()->SetNanColor(originalLut->GetNanColor());
    gradient()->SetUseAboveRangeColor(originalLut->GetUseAboveRangeColor());
    gradient()->SetUseBelowRangeColor(originalLut->GetUseBelowRangeColor());
    gradient()->BuildSpecialColors();
}

bool ColorMapping::currentScalarsUseMappingLegend() const
{
    auto & scalars = currentScalars();

    return (scalars.dataMinValue() != scalars.dataMaxValue())
        && !scalars.usesOwnLookupTable();
}

ColorBarRepresentation & ColorMapping::colorBarRepresentation()
{
    if (!m_colorBarRepresenation)
    {
        m_colorBarRepresenation = std::make_unique<ColorBarRepresentation>(*this);
    }

    return *m_colorBarRepresenation;
}

void ColorMapping::updateAvailableScalars()
{
    // rerun the factory and update the GUI
    setVisualizedData(m_visualizedData);

    emit scalarsChanged();
}
