#include "AttributeArrayComponentMapping.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <QMap>
#include <QDebug>

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
const QString s_name = "attribute array component";
}

const bool AttributeArrayComponentMapping::s_registered = ScalarsForColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<ScalarsForColorMapping *> AttributeArrayComponentMapping::newInstances(const QList<DataObject *> & dataObjects)
{
    QMap<QString, int> arrayNamesComponents;

    // list all available array names, check for same number of components
    for (DataObject * dataObject : dataObjects)
    {
        vtkCellData * cellData = dataObject->dataSet()->GetCellData();
        const int numArrays = cellData->GetNumberOfArrays();
        for (vtkIdType i = 0; i < numArrays; ++i)
        {
            vtkFloatArray * dataArray = vtkFloatArray::SafeDownCast(cellData->GetArray(i));
            if (!dataArray)
                continue;

            QString name = QString::fromLatin1(dataArray->GetName());
            int lastNumComp = arrayNamesComponents.value(name, 0);
            int currentNumComp = dataArray->GetNumberOfComponents();

            if (lastNumComp && lastNumComp != currentNumComp)
            {
                qDebug() << "found array named" << name << "with different number of components (" << lastNumComp << "," << dataArray->GetNumberOfComponents() << ")";
                continue;
            }

            if (!lastNumComp)
                arrayNamesComponents.insert(name, currentNumComp);
        }
    }

    QList<ScalarsForColorMapping *> instances;
    for (auto it = arrayNamesComponents.begin(); it != arrayNamesComponents.end(); ++it)
    {
        for (vtkIdType component = 0; component < it.value(); ++component)
        {
            AttributeArrayComponentMapping * mapping = new AttributeArrayComponentMapping(dataObjects, it.key(), component);
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

AttributeArrayComponentMapping::AttributeArrayComponentMapping(const QList<DataObject *> & dataObjects, QString dataArrayName, vtkIdType component)
    : ScalarsForColorMapping(dataObjects)
    , m_dataArrayName(dataArrayName)
    , m_component(component)
    , m_arrayNumComponents()
{
    m_valid = false;

    assert(!dataObjects.isEmpty());

    QByteArray c_name = dataArrayName.toLatin1();

    // assuming that all data objects have this array and that all arrays have the same number of components
    vtkDataArray * anArray = dataObjects.first()->dataSet()->GetCellData()->GetArray(c_name.data());
    if (anArray)
        m_arrayNumComponents = anArray->GetNumberOfComponents();

    double totalRange[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };
    for (DataObject * dataObject : dataObjects)
    {
        vtkFloatArray * dataArray = vtkFloatArray::SafeDownCast(
            dataObject->dataSet()->GetCellData()->GetArray(c_name.data()));
        
        if (!dataArray)
        {
            qDebug() << "Data array" << dataArrayName << "does not exist in" << dataObject->name();
            continue;
        }

        double range[2];
        dataArray->GetRange(range, m_component);
        totalRange[0] = std::min(totalRange[0], range[0]);
        totalRange[1] = std::max(totalRange[1], range[1]);
    }

    assert(totalRange[0] <= totalRange[1]);

    // discard vector components with constant value
    m_valid = totalRange[0] != totalRange[1];
}

AttributeArrayComponentMapping::~AttributeArrayComponentMapping() = default;

QString AttributeArrayComponentMapping::name() const
{
    if (m_arrayNumComponents == 1)
        return m_dataArrayName;

    QString component = m_arrayNumComponents <= 3
        ? QChar::fromLatin1('x' + m_component)
        : QString::number(m_component);
    
    return m_dataArrayName + " (" + component + ")";
}

vtkAlgorithm * AttributeArrayComponentMapping::createFilter()
{
    vtkAssignAttribute * filter = vtkAssignAttribute::New();

    QByteArray c_name = m_dataArrayName.toLatin1();

    filter->Assign(c_name.data(), vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::CELL_DATA);

    return filter;
}

bool AttributeArrayComponentMapping::usesFilter() const
{
    return true;
}

void AttributeArrayComponentMapping::configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper)
{
    ScalarsForColorMapping::configureDataObjectAndMapper(dataObject, mapper);
    QByteArray c_name = m_dataArrayName.toLatin1();

    // TODO this is marked as legacy, but accessing the mappers LUT is not safe here
    mapper->ColorByArrayComponent(c_name.data(), m_component);
}

void AttributeArrayComponentMapping::updateBounds()
{
    QByteArray c_name = m_dataArrayName.toLatin1();

    double totalRange[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };
    for (DataObject * dataObject : m_dataObjects)
    {
        vtkDataArray * dataArray = dataObject->dataSet()->GetCellData()->GetArray(c_name.data());

        if (!dataArray)
            continue;

        double range[2];
        dataArray->GetRange(range, m_component);
        totalRange[0] = std::min(totalRange[0], range[0]);
        totalRange[1] = std::max(totalRange[1], range[1]);
    }

    m_dataMinValue = totalRange[0];
    m_dataMaxValue = totalRange[1];

    ScalarsForColorMapping::updateBounds();
}

bool AttributeArrayComponentMapping::isValid() const
{
    return m_valid;
}