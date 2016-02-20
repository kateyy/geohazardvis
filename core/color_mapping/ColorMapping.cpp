#include "ColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorBarRepresentation.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/color_mapping/GlyphColorMappingGlyphListener.h>
#include <core/color_mapping/GradientResourceManager.h>


ColorMapping::ColorMapping()
    : QObject()
    , m_glyphListener{ std::make_unique<GlyphColorMappingGlyphListener>() }
    , m_gradient{ vtkSmartPointer<vtkLookupTable>::New() }
{
    connect(m_glyphListener.get(), &GlyphColorMappingGlyphListener::glyphMappingChanged,
        this, &ColorMapping::updateAvailableScalars);
}

ColorMapping::~ColorMapping() = default;

void ColorMapping::setVisualizedData(const QList<AbstractVisualizedData *> & visualizedData)
{
    // clean up old scalars
    for (AbstractVisualizedData * vis : m_visualizedData)
    {
        vis->setScalarsForColorMapping(nullptr);
        disconnect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    auto lastScalars = currentScalarsName();
    m_currentScalarsName.clear();
    m_data.clear();


    m_visualizedData = visualizedData;

    for (AbstractVisualizedData * vis : m_visualizedData)
    {
        // pass our (persistent) gradient object
        vis->setColorMappingGradient(gradient());

        connect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    m_data = ColorMappingRegistry::instance().createMappingsValidFor(visualizedData);
    for (auto & pair : m_data)
    {
        auto & scalars = pair.second;
        scalars->setLookupTable(gradient());
    }

    // disable color mapping if we couldn't find appropriate data
    if (m_data.empty())
    {
        setCurrentScalarsByName("");
        return;
    }

    QString newScalarsName;
    // reuse last configuration if possible
    if (m_data.find(lastScalars) != m_data.end())
        newScalarsName = lastScalars;
    else
    {
        if (m_data.find("user-defined color") != m_data.end())
            newScalarsName = "user-defined color";
        else
            newScalarsName = m_data.begin()->first; // ordered by QString::operator<
    }

    setCurrentScalarsByName(newScalarsName);

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

const QList<AbstractVisualizedData *> & ColorMapping::visualizedData() const
{
    return m_visualizedData;
}

QStringList ColorMapping::scalarsNames() const
{
    QStringList names;
    for (auto & s : m_data)
        names << s.first;

    return names;
}

const QString & ColorMapping::currentScalarsName() const
{
    return m_currentScalarsName;
}

void ColorMapping::setCurrentScalarsByName(const QString & scalarsName)
{
    if (m_currentScalarsName == scalarsName)
    {
        return;
    }

    ColorMappingData * oldScalars = currentScalars();
    if (oldScalars)
        oldScalars->deactivate();

    // cleanup old mappings
    for (AbstractVisualizedData * vis : m_visualizedData)
        vis->setScalarsForColorMapping(nullptr);

    m_currentScalarsName = scalarsName;

    ColorMappingData * scalars = currentScalars();
    if (scalars)
        scalars->activate();

    emit currentScalarsChanged();
}

ColorMappingData * ColorMapping::currentScalars()
{
    return const_cast<ColorMappingData *>(    // don't implement the same function twice
        (static_cast<const ColorMapping *>(this))->currentScalars());
}

const ColorMappingData * ColorMapping::currentScalars() const
{
    if (currentScalarsName().isEmpty())
        return nullptr;

    auto it = m_data.find(currentScalarsName());
    assert(it != m_data.end());
    if (it == m_data.end())
        return nullptr;

    return it->second.get();
}

vtkLookupTable * ColorMapping::gradient()
{
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

    m_gradient->SetTable(originalLut->GetTable());
    m_gradient->SetNanColor(originalLut->GetNanColor());
    m_gradient->SetUseAboveRangeColor(originalLut->GetUseAboveRangeColor());
    m_gradient->SetUseBelowRangeColor(originalLut->GetUseBelowRangeColor());
}

bool ColorMapping::currentScalarsUseMappingLegend() const
{
    const auto scalars = currentScalars();
    if (!scalars)
        return false;

    return scalars->dataMinValue() != scalars->dataMaxValue();
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
