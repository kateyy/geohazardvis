#include "ScalarToColorMapping.h"

#include <cassert>

#include "ScalarsForColorMapping.h"
#include "DefaultColorMapping.h"
#include "CoordinateValueMapping.h"
#include "core/data_objects/RenderedData.h"


ScalarToColorMapping::ScalarToColorMapping()
    : m_gradient(nullptr)
{
    clear();


    // HACK to enforce object file linking
    DefaultColorMapping a({});
    CoordinateXValueMapping x({});
}

void ScalarToColorMapping::setRenderedData(const QList<RenderedData *> & renderedData)
{
    clear();

    m_renderedData = renderedData;

    QList<DataObject *> dataObjects;
    for (RenderedData * rendered : renderedData)
    {
        dataObjects << rendered->dataObject();
    }

    m_scalars = ScalarsForColorMapping::createMappingsValidFor(dataObjects);
}

void ScalarToColorMapping::clear()
{
    m_currentScalars = QString();
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
    return m_currentScalars;
}

void ScalarToColorMapping::setCurrentScalarsByName(QString scalarsName)
{
    if (m_currentScalars == scalarsName)
        return;

    m_currentScalars = scalarsName;

    ScalarsForColorMapping * scalars = nullptr;

    if (!m_currentScalars.isEmpty())
    {
        scalars = m_scalars.value(scalarsName, nullptr);
        assert(scalars);
    }

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyScalarsForColorMapping(scalars);
    }
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
