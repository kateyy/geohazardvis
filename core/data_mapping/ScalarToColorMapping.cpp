#include "ScalarToColorMapping.h"

#include <cassert>

#include "ScalarsForColorMapping.h"
#include "ScalarsForColorMappingRegistry.h"

#include "core/data_objects/RenderedData.h"


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

    for (RenderedData * rendered : renderedData)
    {
        rendered->applyScalarsForColorMapping(m_currentScalars());
        rendered->applyColorGradient(gradient());
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

const QImage * ScalarToColorMapping::gradient() const
{
    return m_gradient;
}

void ScalarToColorMapping::setGradient(const QImage * gradientImage)
{
    m_gradient = gradientImage;

    if (!m_gradient)
        return;

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyColorGradient(m_gradient);
    }
}