#include "ScalarToColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>

#include "ScalarsForColorMapping.h"
#include "ScalarsForColorMappingRegistry.h"

#include <core/data_objects/RenderedData.h>


ScalarToColorMapping::ScalarToColorMapping()
    : m_gradient(nullptr)
{
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

    qDeleteAll(m_scalars);
    m_scalars = ScalarsForColorMappingRegistry::instance().createMappingsValidFor(dataObjects);

    m_currentScalarsName = m_scalars.first()->name();

    for (RenderedData * rendered : renderedData)
    {
        rendered->applyScalarsForColorMapping(m_currentScalars());
        rendered->applyGradientLookupTable(gradient());
    }
}

void ScalarToColorMapping::clear()
{
    m_currentScalarsName = QString();
    m_gradient = nullptr;

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
        scalars = m_scalars.value(scalarsName, nullptr);
        assert(scalars);
    }

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyScalarsForColorMapping(scalars);
    }
}

ScalarsForColorMapping * ScalarToColorMapping::m_currentScalars()
{
    if (currentScalarsName().isEmpty())
        return nullptr;

    auto * scalars = m_scalars.value(currentScalarsName());
    assert(scalars);

    return scalars;
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

void ScalarToColorMapping::setGradient(vtkLookupTable * gradient)
{
    m_gradient = gradient;

    if (!m_gradient)
        return;

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyGradientLookupTable(m_gradient);
    }
}
