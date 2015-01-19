#include "ColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>

#include <core/AbstractVisualizedData.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/color_mapping/ColorMappingRegistry.h>


ColorMapping::ColorMapping()
    : m_gradient(vtkSmartPointer<vtkLookupTable>::New())
    , m_originalGradient(nullptr)
    , m_colorMappingLegend(vtkSmartPointer<vtkScalarBarActor>::New())
    , m_colorMappingLegendVisible(true)
{
    m_colorMappingLegend->SetLookupTable(m_gradient);
    m_colorMappingLegend->SetVisibility(false);

    clear();

    connect(&DataSetHandler::instance(), &DataSetHandler::rawVectorsChanged,
        this, &ColorMapping::updateAvailableScalars);
}

ColorMapping::~ColorMapping()
{
    qDeleteAll(m_scalars);
}

void ColorMapping::setVisualizedData(const QList<AbstractVisualizedData *> & visualizedData)
{
    // clean up old scalars
    for (AbstractVisualizedData * vis : m_visualizedData)
        vis->setScalarsForColorMapping(nullptr);

    QString lastScalars = currentScalarsName();
    m_currentScalarsName.clear();
    qDeleteAll(m_scalars);


    m_visualizedData = visualizedData;

    for (AbstractVisualizedData * vis : m_visualizedData)
    {
        DataObject * dataObject = vis->dataObject();
        // pass our (persistent) gradient object
        vis->setColorMappingGradient(m_gradient);

        connect(dataObject, &DataObject::attributeArraysChanged, this, &ColorMapping::updateAvailableScalars);
    }

    m_scalars = ColorMappingRegistry::instance().createMappingsValidFor(visualizedData);
    for (ColorMappingData * scalars : m_scalars)
    {
        scalars->setLookupTable(m_gradient);
        connect(scalars, &ColorMappingData::dataMinMaxChanged,
            this, &ColorMapping::scalarsChanged);
    }

    // disable color mapping if we couldn't find appropriate data
    if (m_scalars.isEmpty())
    {
        updateLegendVisibility();
        return;
    }

    QString newScalarsName;
    // reuse last configuration if possible
    if (m_scalars.contains(lastScalars))
        newScalarsName = lastScalars;
    else
    {
        if (m_scalars.contains("user-defined color"))
            newScalarsName = "user-defined color";
        else
            newScalarsName = m_scalars.first()->name();
    }

    setCurrentScalarsByName(newScalarsName);
}

void ColorMapping::clear()
{
    m_currentScalarsName = QString();

    m_visualizedData.clear();

    qDeleteAll(m_scalars.values());
    m_scalars.clear();
}

QList<QString> ColorMapping::scalarsNames() const
{
    return m_scalars.keys();
}

QString ColorMapping::currentScalarsName() const
{
    return m_currentScalarsName;
}

void ColorMapping::setCurrentScalarsByName(QString scalarsName)
{
    m_currentScalarsName = scalarsName;

    ColorMappingData * scalars = currentScalars();
    if (scalars)
    {
        scalars->beforeRendering();
    }

    m_colorMappingLegend->SetTitle(scalarsName.toUtf8().data());

    updateLegendVisibility();

    for (AbstractVisualizedData * vis: m_visualizedData)
    {
        vis->setScalarsForColorMapping(scalars);
    }
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

    auto * scalars = m_scalars.value(currentScalarsName());
    assert(scalars);

    return scalars;
}

void ColorMapping::scalarsSetDataComponent(vtkIdType component)
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

    m_gradient->SetTable(gradient->GetTable());
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