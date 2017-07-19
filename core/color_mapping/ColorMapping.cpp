/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ColorMapping.h"

#include <algorithm>
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
    , m_useManualGradient{ false }
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

void ColorMapping::setVisualizedData(const std::vector<AbstractVisualizedData *> & visualizedData)
{
    // clean up old scalars
    for (auto vis : m_visualizedData)
    {
        vis->setScalarsForColorMapping(nullptr);
        disconnect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    const auto lastScalars = m_currentScalarsName;
    m_currentScalarsName.clear();
    releaseMappingData();


    m_visualizedData = visualizedData;

    for (auto vis : m_visualizedData)
    {
        // pass our (persistent) gradient object
        vis->setColorMappingGradient(gradient());

        connect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    if (!m_visualizedData.empty())
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
    if (std::find(m_visualizedData.begin(), m_visualizedData.end(), visualizedData) != m_visualizedData.end())
    {
        return;
    }

    auto newList = m_visualizedData;
    newList.emplace_back(visualizedData);

    setVisualizedData(newList);

    assert(std::find(m_visualizedData.begin(), m_visualizedData.end(), visualizedData) != m_visualizedData.end());

    emit visualizedDataChanged();
}

void ColorMapping::unregisterVisualizedData(AbstractVisualizedData * visualizedData)
{
    const auto visIt = std::find(m_visualizedData.begin(), m_visualizedData.end(), visualizedData);
    if (visIt == m_visualizedData.end())
    {
        return;
    }

    const auto index = static_cast<size_t>(visIt - m_visualizedData.begin());

    auto newList = m_visualizedData;
    newList.erase(newList.begin() + index);
    assert(std::find(newList.begin(), newList.end(), visualizedData) == newList.end());

    setVisualizedData(newList);

    assert(std::find(m_visualizedData.begin(), m_visualizedData.end(), visualizedData) == m_visualizedData.end());

    emit visualizedDataChanged();
}

bool ColorMapping::scalarsAvailable() const
{
    return !m_data.empty();
}

const std::vector<AbstractVisualizedData *> & ColorMapping::visualizedData() const
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

std::vector<ColorMappingData *> ColorMapping::scalars()
{
    std::vector<ColorMappingData *> result;
    for (auto & s : m_data)
    {
        result.emplace_back(s.second.get());
    }
    return result;
}

std::vector<const ColorMappingData*> ColorMapping::scalars() const
{
    std::vector<const ColorMappingData *> result;
    for (auto & s : m_data)
    {
        result.emplace_back(s.second.get());
    }
    return result;
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

const ColorMappingData * ColorMapping::scalarsByName(const QString & scalarsName) const
{
    const auto it = m_data.find(scalarsName);
    return it == m_data.end() ? nullptr : it->second.get();
}

ColorMappingData * ColorMapping::scalarsByName(const QString & scalarsName)
{
    const auto it = m_data.find(scalarsName);
    return it == m_data.end() ? nullptr : it->second.get();
}

const ColorMappingData & ColorMapping::currentScalars() const
{
    if (!m_isEnabled || m_currentScalarsName.isEmpty())
    {
        return nullColorMapping();
    }

    auto it = m_data.find(m_currentScalarsName);
    assert(it != m_data.end());
    if (it == m_data.end())
    {
        qDebug() << "Invalid scalars requested: " << m_currentScalarsName;
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

    auto lut = GradientResourceManager::instance().gradient(m_gradientName).lookupTable;

    applyGradient(*lut);
}

void ColorMapping::setManualGradient(vtkLookupTable & gradient)
{
    m_useManualGradient = true;

    applyGradient(gradient);
}

void ColorMapping::applyGradient(vtkLookupTable & gradient)
{
    auto ownGradient = this->gradient();
    ownGradient->SetTable(gradient.GetTable());
    ownGradient->SetNanColor(gradient.GetNanColor());
    ownGradient->SetUseAboveRangeColor(gradient.GetUseAboveRangeColor());
    ownGradient->SetUseBelowRangeColor(gradient.GetUseBelowRangeColor());
    ownGradient->BuildSpecialColors();
}

void ColorMapping::setUseDefaultGradients()
{
    m_useManualGradient = false;

    auto lut = GradientResourceManager::instance().gradient(m_gradientName).lookupTable;

    applyGradient(*lut);
}

bool ColorMapping::usesManualGradient() const
{
    return m_useManualGradient;
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
