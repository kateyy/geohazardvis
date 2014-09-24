#include "RawArrayComponentMapping.h"

#include <cassert>
#include <algorithm>

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkMapper.h>
#include <vtkAssignAttribute.h>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <core/vtkhelper.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/AttributeVectorData.h>
#include <core/data_objects/DataObject.h>
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

    for (AttributeVectorData * attr : DataSetHandler::instance().attributeVectors())
    {
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
    : AbstractArrayComponentMapping(dataObjects, QString::fromLatin1(dataArray->GetName()), component)
    , m_dataArray(dataArray)
{
    assert(dataArray);
    assert(dataArray->GetNumberOfComponents() > m_component);

    m_arrayNumComponents = dataArray->GetNumberOfComponents();

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

    m_isValid = true;
}

RawArrayComponentMapping::~RawArrayComponentMapping()
{
    if (!m_isValid)
        return;

    for (DataObject * dataObject : m_dataObjects)
    {
        dataObject->dataSet()->GetCellData()->RemoveArray(
            arraySectionName(dataObject).data());
    }
}

vtkIdType RawArrayComponentMapping::maximumStartingIndex()
{
    vtkIdType indexes = 0;
    for (DataObject * dataObject : m_dataObjects)
        indexes += dataObject->dataSet()->GetNumberOfCells();

    vtkIdType diff = m_dataArray->GetNumberOfTuples() - indexes;

    assert(diff >= 0);

    return diff;
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

    vtkIdType secIndex = sectionIndex(dataObject);
    QByteArray sectionName = arraySectionName(dataObject);
    vtkIdType sectionSize = dataObject->dataSet()->GetNumberOfCells();
    vtkIdType numComponents = m_dataArray->GetNumberOfComponents();

    // create array that reuses a data section of our data array
    VTK_CREATE(vtkFloatArray, section);
    section->GetInformation()->Set(DataObject::ArrayIsAuxiliaryKey(), true);
    section->SetName(sectionName.data());
    section->SetNumberOfComponents(numComponents);
    section->SetNumberOfTuples(sectionSize);
    section->SetArray(
        m_dataArray->GetPointer(secIndex * numComponents),
        sectionSize * numComponents, true);

    m_sections.insert(dataObject, section);

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

void RawArrayComponentMapping::startingIndexChangedEvent()
{
    redistributeArraySections();
}

void RawArrayComponentMapping::objectOrderChangedEvent()
{
    redistributeArraySections();
}

QByteArray RawArrayComponentMapping::arraySectionName(DataObject * dataObject)
{
    return (QString::fromLatin1(m_dataArray->GetName()) + "_" + QString::number(reinterpret_cast<size_t>(dataObject))).toLatin1();
}

vtkIdType RawArrayComponentMapping::sectionIndex(DataObject * dataObject)
{
    return startingIndex() + m_dataObjectToArrayIndex[dataObject];
}

void RawArrayComponentMapping::redistributeArraySections()
{
    vtkIdType currentIndex = 0;
    for (DataObject * dataObject : m_dataObjects)
    {
        m_dataObjectToArrayIndex.insert(dataObject, currentIndex);
        currentIndex += dataObject->dataSet()->GetNumberOfCells();
    }
    assert(currentIndex <= m_dataArray->GetNumberOfTuples());

    vtkIdType numComponents = m_dataArray->GetNumberOfComponents();
    for (auto it = m_sections.begin(); it != m_sections.end(); ++it)
    {
        DataObject * dataObject = it.key();
        vtkSmartPointer<vtkFloatArray> & array = it.value();
        array->SetArray(
            m_dataArray->GetPointer(sectionIndex(dataObject) * numComponents),
            dataObject->dataSet()->GetNumberOfCells() * numComponents,
            true);

        dataObject->dataSet()->GetCellData()->RemoveArray(array->GetName());
        dataObject->dataSet()->GetCellData()->AddArray(array);
    }
}
