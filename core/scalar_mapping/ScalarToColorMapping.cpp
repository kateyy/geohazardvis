#include "ScalarToColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>

#include "ScalarsForColorMapping.h"
#include "ScalarsForColorMappingRegistry.h"

#include <core/data_objects/RenderedData.h>


ScalarToColorMapping::ScalarToColorMapping()
    : m_gradient(vtkSmartPointer<vtkLookupTable>::New())
    , m_originalGradient(nullptr)
    , m_colorMappingLegend(vtkSmartPointer<vtkScalarBarActor>::New())
{
    m_colorMappingLegend->SetLookupTable(m_gradient);
    m_colorMappingLegend->SetVisibility(false);

    clear();
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

    qDeleteAll(m_scalars);
    m_scalars = ScalarsForColorMappingRegistry::instance().createMappingsValidFor(dataObjects);

    for (ScalarsForColorMapping * scalars : m_scalars)
    {
        connect(scalars, &ScalarsForColorMapping::minMaxChanged,
            this, &ScalarToColorMapping::updateGradientValueRange);
    }

    assert(!m_scalars.isEmpty());   // we should we something applicable here

    QString newScalarsName;
    // reuse last configuration if possible
    if (m_scalars.contains(lastScalars))
        newScalarsName = lastScalars;
    else
        newScalarsName = m_scalars.first()->name();

    setCurrentScalarsByName(newScalarsName);

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
    if (m_currentScalarsName == scalarsName)
        return;

    m_currentScalarsName = scalarsName;

    ScalarsForColorMapping * scalars = nullptr;

    if (!m_currentScalarsName.isEmpty())
    {
        scalars = m_scalars.value(m_currentScalarsName, nullptr);
        assert(scalars);
    }

    m_colorMappingLegend->SetTitle(scalarsName.toLatin1().data());
    bool usesGradient = scalars->dataMinValue() != scalars->dataMaxValue();
    m_colorMappingLegend->SetVisibility(usesGradient);

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyScalarsForColorMapping(scalars);
    }
}

ScalarsForColorMapping * ScalarToColorMapping::currentScalars()
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
    return m_colorMappingLegend->GetVisibility() != 0;
}

void ScalarToColorMapping::updateGradientValueRange()
{
    m_gradient->SetTableRange(currentScalars()->minValue(), currentScalars()->maxValue());
}
