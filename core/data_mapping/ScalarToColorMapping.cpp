#include "ScalarToColorMapping.h"

#include <cassert>

#include "ScalarsForColorMapping.h"
#include "DefaultColorMapping.h"
#include "core/data_objects/RenderedData.h"


ScalarToColorMapping::ScalarToColorMapping()
    : m_gradient(nullptr)
{
    clear();


    // HACK to enforce object file linking
    DefaultColorMapping a({});
}

void ScalarToColorMapping::setRenderedData(const QList<RenderedData *> & renderedData)
{
    clear();

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

QString ScalarToColorMapping::currentScalars() const
{
    return m_currentScalars;
}

void ScalarToColorMapping::setCurrentScalars(QString scalarsName)
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

const QImage * ScalarToColorMapping::gradient() const
{
    return m_gradient;
}

void ScalarToColorMapping::setGradient(const QImage * gradientImage)
{
    if (m_gradient = gradientImage)
        return;

    m_gradient = gradientImage;

    if (!m_gradient)
        return;

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->applyColorGradient(m_gradient);
    }
}
