#include "AttributeArrayComponentMapping.h"

#include <cassert>

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkCellData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/AttributeVectorData.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


namespace
{
const QString s_name = "attribute array component";
}

const bool AttributeArrayComponentMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<ScalarsForColorMapping *> AttributeArrayComponentMapping::newInstances(const QList<DataObject *> & dataObjects)
{
    QList<AttributeVectorData *> attrs;

    // only for one object currently
    if (dataObjects.size() != 1)
        return{};

    DataObject * inputObject = dataObjects.first();

    for (DataObject * dataObject : DataSetHandler::instance().dataObjects())
    {
        AttributeVectorData * attr = dynamic_cast<AttributeVectorData *>(dataObject);
        if (!attr)
            continue;

        if (attr->dataArray()->GetNumberOfTuples() >= inputObject->dataSet()->GetNumberOfCells())
            attrs << attr;
    }

    QList<ScalarsForColorMapping *> instances;
    for (AttributeVectorData * attr : attrs)
    {
        for (vtkIdType component = 0; component < attr->dataArray()->GetNumberOfComponents(); ++component)
        {
            AttributeArrayComponentMapping * mapping = new AttributeArrayComponentMapping(dataObjects, attr, component);
            if (mapping->isValid())
            {
                mapping->initialize();
                instances << mapping;
            }
            else
                delete mapping;
        }
    }

    return instances;
}

AttributeArrayComponentMapping::AttributeArrayComponentMapping(const QList<DataObject *> & dataObjects, AttributeVectorData * attributeVector, vtkIdType component)
    : ScalarsForColorMapping(dataObjects)
    , m_attributeVector(attributeVector)
    , m_component(component)
{
    assert(attributeVector);
    assert(attributeVector->dataArray()->GetNumberOfComponents() > m_component);

    m_valid = false;

    assert(dataObjects.size() == 1);
    m_polyData = vtkPolyData::SafeDownCast(dataObjects[0]->dataSet());

    if (!m_polyData)
        return;

    double range[2];
    m_attributeVector->dataArray()->GetRange(range, m_component);

    // discard vector components with constant value
    m_valid = range[0] != range[1];
}

void AttributeArrayComponentMapping::initialize()
{
    ScalarsForColorMapping::initialize();

    m_polyData->GetCellData()->SetScalars(m_attributeVector->dataArray());
}

AttributeArrayComponentMapping::~AttributeArrayComponentMapping() = default;

QString AttributeArrayComponentMapping::name() const
{
    QString baseName = QString::fromLatin1(m_attributeVector->dataArray()->GetName());
    int numComponents = m_attributeVector->dataArray()->GetNumberOfComponents();

    if (numComponents == 0)
        return baseName;

    QString component = numComponents <= 3
        ? QChar::fromLatin1('x' + m_component)
        : QString::number(m_component);
    
    return  baseName + " (" + component + ")";
}

void AttributeArrayComponentMapping::configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * /*mapper*/)
{
    dataObject->dataSet()->GetCellData()->SetScalars(m_attributeVector->dataArray());
}

void AttributeArrayComponentMapping::updateBounds()
{
    double range[2];
    m_attributeVector->dataArray()->GetRange(range, m_component);

    m_dataMinValue = range[0];
    m_dataMaxValue = range[1];

    ScalarsForColorMapping::updateBounds();
}

bool AttributeArrayComponentMapping::isValid() const
{
    return m_valid;
}
