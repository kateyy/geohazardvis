#include "ColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/color_mapping/GlyphColorMappingGlyphListener.h>
#include <core/utility/ScalarBarActor.h>


ColorMapping::ColorMapping(QObject * parent)
    : QObject(parent)
    , m_glyphListener(std::make_unique<GlyphColorMappingGlyphListener>())
    , m_gradient(vtkSmartPointer<vtkLookupTable>::New())
    , m_originalGradient(nullptr)
    , m_colorMappingLegend(vtkSmartPointer<OrientedScalarBarActor>::New())
    , m_colorMappingLegendVisible(true)
{
    m_colorMappingLegend->SetLookupTable(m_gradient);
    m_colorMappingLegend->SetVisibility(false);

    clear();

    connect(m_glyphListener.get(), &GlyphColorMappingGlyphListener::glyphMappingChanged,
        this, &ColorMapping::updateAvailableScalars, Qt::QueuedConnection);
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

    QString lastScalars = currentScalarsName();
    m_currentScalarsName.clear();
    m_scalars.clear();


    m_visualizedData = visualizedData;

    for (AbstractVisualizedData * vis : m_visualizedData)
    {
        // pass our (persistent) gradient object
        vis->setColorMappingGradient(m_gradient);

        connect(&vis->dataObject(), &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars, Qt::QueuedConnection);
    }

    m_scalars = ColorMappingRegistry::instance().createMappingsValidFor(visualizedData);
    for (auto & pair : m_scalars)
    {
        auto & scalars = pair.second;
        scalars->setLookupTable(m_gradient);
        connect(scalars.get(), &ColorMappingData::dataMinMaxChanged,
            this, &ColorMapping::scalarsChanged);
    }

    // disable color mapping if we couldn't find appropriate data
    if (m_scalars.empty())
    {
        updateLegendVisibility();
        return;
    }

    QString newScalarsName;
    // reuse last configuration if possible
    if (m_scalars.find(lastScalars) != m_scalars.end())
        newScalarsName = lastScalars;
    else
    {
        if (m_scalars.find("user-defined color") != m_scalars.end())
            newScalarsName = "user-defined color";
        else
            newScalarsName = m_scalars.begin()->first; // ordered by QString::operator<
    }

    setCurrentScalarsByName(newScalarsName);

    m_glyphListener->setData(visualizedData);
}

void ColorMapping::clear()
{
    m_currentScalarsName = QString();

    m_visualizedData.clear();

    m_scalars.clear();
}

QList<QString> ColorMapping::scalarsNames() const
{
    QList<QString> names;
    for (auto & s : m_scalars)
        names << s.first;

    return names;
}

QString ColorMapping::currentScalarsName() const
{
    return m_currentScalarsName;
}

void ColorMapping::setCurrentScalarsByName(const QString & scalarsName)
{
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

    m_colorMappingLegend->SetTitle(scalarsName.toUtf8().data());

    updateLegendVisibility();
}

ColorMappingData * ColorMapping::currentScalars()
{
    return const_cast<ColorMappingData *>(    // don't implement the same function twice
        ((const ColorMapping *)(this))->currentScalars());
}

const ColorMappingData * ColorMapping::currentScalars() const
{
    if (currentScalarsName().isEmpty())
        return nullptr;

    auto it = m_scalars.find(currentScalarsName());
    assert(it != m_scalars.end());
    if (it == m_scalars.end())
        return nullptr;

    return it->second.get();
}

void ColorMapping::scalarsSetDataComponent(int component)
{
    currentScalars()->setDataComponent(component);
}

vtkLookupTable * ColorMapping::gradient()
{
    return m_gradient;
}

vtkLookupTable * ColorMapping::originalGradient()
{
    return m_originalGradient;
}

void ColorMapping::setGradient(vtkLookupTable * gradient)
{
    assert(gradient);

    m_originalGradient = gradient;

    if (gradient)
    {
        m_gradient->SetTable(gradient->GetTable());
        m_gradient->SetNanColor(gradient->GetNanColor());
        m_gradient->SetUseAboveRangeColor(gradient->GetUseAboveRangeColor());
        m_gradient->SetUseBelowRangeColor(gradient->GetUseBelowRangeColor());
    }
    else
    {
        // no gradients loaded, use VTK's default rainbow instead
        m_gradient->SetTable(nullptr);
        m_gradient->Build();
    }
}

vtkScalarBarActor * ColorMapping::colorMappingLegend()
{
    return m_colorMappingLegend;
}

bool ColorMapping::currentScalarsUseMappingLegend() const
{
    const ColorMappingData * scalars = currentScalars();
    if (!scalars)
        return false;

    return scalars->dataMinValue() != scalars->dataMaxValue();
}

bool ColorMapping::colorMappingLegendVisible() const
{
    return m_colorMappingLegendVisible;
}

void ColorMapping::setColorMappingLegendVisible(bool visible)
{
    m_colorMappingLegendVisible = visible;

    updateLegendVisibility();
}

void ColorMapping::updateAvailableScalars()
{
    // rerun the factory and update the GUI
    setVisualizedData(m_visualizedData);

    emit scalarsChanged();
}

void ColorMapping::updateLegendVisibility()
{
    bool actualVisibilty = currentScalarsUseMappingLegend() && m_colorMappingLegendVisible;

    bool oldValue = m_colorMappingLegend->GetVisibility() > 0;

    if (oldValue != actualVisibilty)
    {
        m_colorMappingLegend->SetVisibility(actualVisibilty);
        emit colorLegendVisibilityChanged(actualVisibilty);
    }
}
