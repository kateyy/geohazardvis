#include "AttributeArrayComponentMapping.h"

#include <cassert>

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
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
    QList<vtkFloatArray *> dataArrays;

    // only for one object currently
    if (dataObjects.size() != 1)
        return{};

    DataObject * inputObject = dataObjects.first();

    for (DataObject * dataObject : DataSetHandler::instance().dataObjects())
    {
        AttributeVectorData * attr = dynamic_cast<AttributeVectorData *>(dataObject);
        if (!attr)
            continue;

        vtkFloatArray * dataArray = attr->dataArray();

        if (dataArray->GetNumberOfTuples() >= inputObject->dataSet()->GetNumberOfCells())
            dataArrays << dataArray;
    }

    DataObject * dataObject = dataObjects.first();
    vtkCellData * cellData = dataObject->dataSet()->GetCellData();
    for (vtkIdType i = 0; i < cellData->GetNumberOfArrays(); ++i)
    {
        vtkFloatArray * dataArray = vtkFloatArray::SafeDownCast(cellData->GetArray(i));
        if (dataArray)
            dataArrays << dataArray;
    }

    QList<ScalarsForColorMapping *> instances;
    for (vtkFloatArray * dataArray : dataArrays)
    {
        for (vtkIdType component = 0; component < dataArray->GetNumberOfComponents(); ++component)
        {
            AttributeArrayComponentMapping * mapping = new AttributeArrayComponentMapping(dataObjects, dataArray, component);
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

AttributeArrayComponentMapping::AttributeArrayComponentMapping(const QList<DataObject *> & dataObjects, vtkFloatArray * dataArray, vtkIdType component)
    : ScalarsForColorMapping(dataObjects)
    , m_dataArray(dataArray)
    , m_component(component)
{
    assert(dataArray);
    assert(dataArray->GetNumberOfComponents() > m_component);

    m_valid = false;

    assert(dataObjects.size() == 1);

    double range[2];
    dataArray->GetRange(range, m_component);

    // discard vector components with constant value
    m_valid = range[0] != range[1];
}

AttributeArrayComponentMapping::~AttributeArrayComponentMapping() = default;

QString AttributeArrayComponentMapping::name() const
{
    QString baseName = QString::fromLatin1(m_dataArray->GetName());
    int numComponents = m_dataArray->GetNumberOfComponents();

    if (numComponents == 0)
        return baseName;

    QString component = numComponents <= 3
        ? QChar::fromLatin1('x' + m_component)
        : QString::number(m_component);
    
    return  baseName + " (" + component + ")";
}

void AttributeArrayComponentMapping::configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper)
{
    dataObject->dataSet()->GetCellData()->SetScalars(m_dataArray);
}

void AttributeArrayComponentMapping::updateBounds()
{
    double range[2];
    m_dataArray->GetRange(range, m_component);

    m_dataMinValue = range[0];
    m_dataMaxValue = range[1];

    ScalarsForColorMapping::updateBounds();
}

bool AttributeArrayComponentMapping::isValid() const
{
    return m_valid;
}
