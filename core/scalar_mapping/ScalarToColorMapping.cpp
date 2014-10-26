#include "ScalarToColorMapping.h"

#include <cassert>

#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/RenderedData.h>
#include <core/scalar_mapping/ScalarsForColorMapping.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


ScalarToColorMapping::ScalarToColorMapping()
    : m_gradient(vtkSmartPointer<vtkLookupTable>::New())
    , m_originalGradient(nullptr)
    , m_colorMappingLegend(vtkSmartPointer<vtkScalarBarActor>::New())
    , m_colorMappingLegendVisible(true)
{
    m_colorMappingLegend->SetLookupTable(m_gradient);
    m_colorMappingLegend->SetVisibility(false);

    clear();

    connect(&DataSetHandler::instance(), &DataSetHandler::rawVectorsChanged,
        this, &ScalarToColorMapping::updateAvailableScalars);
}

ScalarToColorMapping::~ScalarToColorMapping()
{
    qDeleteAll(m_scalars);
}

void ScalarToColorMapping::setRenderedData(const QList<RenderedData *> & renderedData)
{
    // clean up old scalars
    for (RenderedData * renderedData : m_renderedData)
        renderedData->setScalarsForColorMapping(nullptr);

    QString lastScalars = currentScalarsName();
    m_currentScalarsName.clear();
    qDeleteAll(m_scalars);


    m_renderedData = renderedData;

    QList<DataObject *> dataObjects;
    for (RenderedData * renderedData : m_renderedData)
    {
        dataObjects << renderedData->dataObject();
        // pass our (persistent) gradient object
        renderedData->setColorMappingGradient(m_gradient);
    }

    m_scalars = ScalarsForColorMappingRegistry::instance().createMappingsValidFor(dataObjects);
    for (ScalarsForColorMapping * scalars : m_scalars)
    {
        scalars->setLookupTable(m_gradient);
        connect(scalars, &ScalarsForColorMapping::dataMinMaxChanged,
            this, &ScalarToColorMapping::scalarsChanged);
    }

    // disable scalar mapping if we couldn't find appropriate data
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

    ScalarsForColorMapping * scalars = currentScalars();
    if (scalars)
    {
        scalars->beforeRendering();
    }

    QByteArray c_name = scalarsName.toLatin1();
    m_colorMappingLegend->SetTitle(c_name.data());

    updateLegendVisibility();

    for (RenderedData * renderedData : m_renderedData)
    {
        renderedData->setScalarsForColorMapping(scalars);
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

void ScalarToColorMapping::scalarsSetDataComponent(vtkIdType component)
{
    currentScalars()->setDataComponent(component);
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

    m_gradient->SetTable(gradient->GetTable());
}

vtkScalarBarActor * ScalarToColorMapping::colorMappingLegend()
{
    return m_colorMappingLegend;
}

bool ScalarToColorMapping::currentScalarsUseMappingLegend() const
{
    const ScalarsForColorMapping * scalars = currentScalars();
    if (!scalars)
        return false;

    return scalars->dataMinValue() != scalars->dataMaxValue();
}

bool ScalarToColorMapping::colorMappingLegendVisible() const
{
    return m_colorMappingLegendVisible;
}

void ScalarToColorMapping::setColorMappingLegendVisible(bool visible)
{
    m_colorMappingLegendVisible = visible;

    updateLegendVisibility();
}

void ScalarToColorMapping::updateAvailableScalars()
{
    // rerun the factory and update the GUI
    setRenderedData(m_renderedData);

    emit scalarsChanged();
}

void ScalarToColorMapping::updateLegendVisibility()
{
    bool actualVisibilty = currentScalarsUseMappingLegend() && m_colorMappingLegendVisible;

    bool oldValue = m_colorMappingLegend->GetVisibility() > 0;

    if (oldValue != actualVisibilty)
    {
        m_colorMappingLegend->SetVisibility(actualVisibilty);
        emit colorLegendVisibilityChanged(actualVisibilty);
    }
}
