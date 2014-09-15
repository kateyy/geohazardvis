#include "RawArrayComponentMapping.h"

#include <cassert>
#include <algorithm>

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkMapper.h>
#include <vtkAssignAttribute.h>

#include <core/vtkhelper.h>
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

    vtkIdType totalNumCells = 0;
    for (DataObject * dataObject : dataObjects)
        totalNumCells += dataObject->dataSet()->GetNumberOfCells();

    for (DataObject * dataObject : DataSetHandler::instance().dataObjects())
    {
        AttributeVectorData * attr = dynamic_cast<AttributeVectorData *>(dataObject);
        if (!attr)
            continue;

        vtkFloatArray * dataArray = attr->dataArray();

        // support only scalars that span all visible objects
        if (dataArray->GetNumberOfTuples() >= totalNumCells)
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

    double range[2];
    dataArray->GetRange(range, m_component);
    // discard vector components with constant value
    if (range[0] == range[1])
        return;

    vtkIdType currentIndex = 0;
    for (DataObject * dataObject : dataObjects)
    {
        m_dataObjectToArrayIndex.insert(dataObject, currentIndex);
        currentIndex += dataObject->dataSet()->GetNumberOfCells();
    }
    assert(currentIndex <= dataArray->GetNumberOfTuples());

    m_valid = true;
}

RawArrayComponentMapping::~RawArrayComponentMapping()
{
    if (!m_valid)
        return;

    for (DataObject * dataObject : m_dataObjects)
    {
        dataObject->dataSet()->GetCellData()->RemoveArray(
            arraySectionName(dataObject).data());
    }
}

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

vtkAlgorithm * RawArrayComponentMapping::createFilter(DataObject * dataObject)
{
    vtkAssignAttribute * filter = vtkAssignAttribute::New();

    filter->Assign(arraySectionName(dataObject).data(), vtkDataSetAttributes::SCALARS,
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

    vtkIdType sectionIndex = m_dataObjectToArrayIndex[dataObject];
    QByteArray sectionName = arraySectionName(dataObject);
    vtkIdType sectionSize = dataObject->dataSet()->GetNumberOfCells();
    vtkIdType numComponents = m_dataArray->GetNumberOfComponents();

    // create array that reuses a data section of our data array
    VTK_CREATE(vtkFloatArray, section);
    section->SetName(sectionName.data());
    section->SetNumberOfComponents(numComponents);
    section->SetNumberOfTuples(sectionSize);
    section->SetArray(
        m_dataArray->GetPointer(sectionIndex * numComponents),
        sectionSize * numComponents, true);

    dataObject->dataSet()->GetCellData()->AddArray(section);
    // TODO this is marked as legacy, but accessing the mappers LUT is not safe here
    mapper->ColorByArrayComponent(sectionName.data(), m_component);
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

QByteArray RawArrayComponentMapping::arraySectionName(DataObject * dataObject)
{
    vtkIdType sectionIndex = m_dataObjectToArrayIndex.value(dataObject, -1);
    assert(sectionIndex >= 0);

    return (QString::fromLatin1(m_dataArray->GetName()) + "_" + QString::number(sectionIndex)).toLatin1();
}
