#include "RawArrayComponentMapping.h"

#include <cassert>

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkMapper.h>
#include <vtkAssignAttribute.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/AttributeVectorData.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/scalar_mapping/ScalarsForColorMappingRegistry.h>


namespace
{
const QString s_name = "raw array component";
}

const bool RawArrayComponentMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<ScalarsForColorMapping *> RawArrayComponentMapping::newInstances(const QList<DataObject *> & dataObjects)
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

    QList<ScalarsForColorMapping *> instances;
    for (vtkFloatArray * dataArray : dataArrays)
    {
        for (vtkIdType component = 0; component < dataArray->GetNumberOfComponents(); ++component)
        {
            RawArrayComponentMapping * mapping = new RawArrayComponentMapping(dataObjects, dataArray, component);
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

RawArrayComponentMapping::RawArrayComponentMapping(const QList<DataObject *> & dataObjects, vtkFloatArray * dataArray, vtkIdType component)
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

RawArrayComponentMapping::~RawArrayComponentMapping() = default;

QString RawArrayComponentMapping::name() const
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

vtkAlgorithm * RawArrayComponentMapping::createFilter()
{
    vtkAssignAttribute * filter = vtkAssignAttribute::New();

    filter->Assign(m_dataArray->GetName(), vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::CELL_DATA);

    return filter;
}

bool RawArrayComponentMapping::usesFilter() const
{
    return true;
}

void RawArrayComponentMapping::configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper)
{
    ScalarsForColorMapping::configureDataObjectAndMapper(dataObject, mapper);
    dataObject->dataSet()->GetCellData()->AddArray(m_dataArray);
    // TODO this is marked as legacy, but accessing the mappers LUT is not safe here
    mapper->ColorByArrayComponent(m_dataArray->GetName(), m_component);
}

void RawArrayComponentMapping::updateBounds()
{
    double range[2];
    m_dataArray->GetRange(range, m_component);

    m_dataMinValue = range[0];
    m_dataMaxValue = range[1];

    ScalarsForColorMapping::updateBounds();
}

bool RawArrayComponentMapping::isValid() const
{
    return m_valid;
}
