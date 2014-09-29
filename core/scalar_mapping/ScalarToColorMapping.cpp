#include "ScalarToColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>

#include "ScalarsForColorMapping.h"
#include "ScalarsForColorMappingRegistry.h"

#include <core/data_objects/RenderedData.h>
#include <core/DataSetHandler.h>


ScalarToColorMapping::ScalarToColorMapping()
    : m_gradient(vtkSmartPointer<vtkLookupTable>::New())
    , m_originalGradient(nullptr)
    , m_colorMappingLegend(vtkSmartPointer<vtkScalarBarActor>::New())
    , m_colorMappingLegendVisible(true)
{
    m_colorMappingLegend->SetLookupTable(m_gradient);
    m_colorMappingLegend->SetVisibility(false);

    clear();

    connect(&DataSetHandler::instance(), &DataSetHandler::attributeVectorsChanged,
        this, &ScalarToColorMapping::updateAvailableScalars);
}

void ScalarToColorMapping::setRenderedData(const QList<RenderedData *> & renderedData)
{
    m_renderedData = renderedData;

    QList<DataObject *> dataObjects;
    for (RenderedData * rendered : renderedData)
    {
        dataObjects << rendered->dataObject();
    }

    QString lastScalars = currentScalarsName();
    m_currentScalarsName.clear();

    qDeleteAll(m_scalars);
    m_scalars = ScalarsForColorMappingRegistry::instance().createMappingsValidFor(dataObjects);

    for (ScalarsForColorMapping * scalars : m_scalars)
    {
        connect(scalars, &ScalarsForColorMapping::minMaxChanged,
            this, &ScalarToColorMapping::updateGradientValueRange);

        connect(scalars, &ScalarsForColorMapping::dataMinMaxChanged,
            this, &ScalarToColorMapping::scalarsChanged);
    }

    if (m_scalars.isEmpty())
        return;

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

    updateGradientValueRange();

    for (RenderedData * rendered : renderedData)
    {
        rendered->applyGradientLookupTable(gradient());
    }
}

void ScalarToColorMapping::clear()
{
    m_currentScalarsName = QString();

    m_renderedData.clear();

    qDeleteAll(m_scalars.values());
    m_scalars.clear();
}

QList<QString> ScalarToColorMapping::scalarsNames() const
{
    return m_scalars.keys();
}

QString ScalarToColorMapping::currentScalarsName() const
{
    return m_currentScalarsName;
}

void ScalarToColorMapping::setCurrentScalarsByName(QString scalarsName)
{
    m_currentScalarsName = scalarsName;

    ScalarsForColorMapping * scalars = nullptr;

    if (!m_currentScalarsName.isEmpty())
    {
        scalars = m_scalars.value(m_currentScalarsName, nullptr);
        assert(scalars);
    }

    QByteArray c_name = scalarsName.toLatin1();
    m_colorMappingLegend->SetTitle(c_name.data());

    setColorMappingLegendVisible(m_colorMappingLegend);

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyScalarsForColorMapping(scalars);
    }
}

ScalarsForColorMapping * ScalarToColorMapping::currentScalars()
{
    return const_cast<ScalarsForColorMapping *>(    // don't implement the same function twice
        ((const ScalarToColorMapping *)(this))->currentScalars());
}

const ScalarsForColorMapping * ScalarToColorMapping::currentScalars() const
{
    if (currentScalarsName().isEmpty())
        return nullptr;

    auto * scalars = m_scalars.value(currentScalarsName());
    assert(scalars);

    return scalars;
}

vtkLookupTable * ScalarToColorMapping::gradient()
{
    return m_gradient;
}

vtkLookupTable * ScalarToColorMapping::originalGradient()
{
    return m_originalGradient;
}

void ScalarToColorMapping::setGradient(vtkLookupTable * gradient)
{
    assert(gradient);

    m_originalGradient = gradient;

    m_gradient->DeepCopy(gradient);

    updateGradientValueRange();

    for (RenderedData * renderedData : m_renderedData)
        renderedData->applyGradientLookupTable(m_gradient);
}

vtkScalarBarActor * ScalarToColorMapping::colorMappingLegend()
{
    return m_colorMappingLegend;
}

bool ScalarToColorMapping::currentScalarsUseMappingLegend() const
{
    const ScalarsForColorMapping * scalars = currentScalars();
    return scalars->dataMinValue() != scalars->dataMaxValue();
}

bool ScalarToColorMapping::colorMappingLegendVisible() const
{
    return m_colorMappingLegendVisible;
}

void ScalarToColorMapping::setColorMappingLegendVisible(bool visible)
{
    m_colorMappingLegendVisible = visible;

    bool actualVisibilty = currentScalarsUseMappingLegend() && m_colorMappingLegendVisible;

    m_colorMappingLegend->SetVisibility(actualVisibilty);

    emit colorLegendVisibilityChanged(actualVisibilty);
}

void ScalarToColorMapping::updateGradientValueRange()
{
    ScalarsForColorMapping * scalars = currentScalars();

    if (scalars)
        m_gradient->SetTableRange(scalars->minValue(), scalars->maxValue());
}

void ScalarToColorMapping::updateAvailableScalars()
{
    // rerun the factory and update the GUI
    setRenderedData(m_renderedData);

    emit scalarsChanged();
}
