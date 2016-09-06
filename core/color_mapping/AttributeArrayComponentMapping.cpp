#include "AttributeArrayComponentMapping.h"

#include <algorithm>
#include <cassert>

#include <QDebug>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <vtkAlgorithmOutput.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>
#include <vtkMapper.h>
#include <vtkAssignAttribute.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/utility/DataExtent.h>


namespace
{
const QString s_name = "attribute array component";
}

const bool AttributeArrayComponentMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> AttributeArrayComponentMapping::newInstances(const QList<AbstractVisualizedData *> & visualizedData)
{
    struct ArrayInfo
    {
        explicit ArrayInfo(int comp = 0)
            : numComponents(comp)
        {
        }
        int numComponents;
        QMap<AbstractVisualizedData *, IndexType> attributeLocations;
    };

    auto checkAttributeArrays = [] (
        AbstractVisualizedData * vis, vtkDataSetAttributes * attributes, 
        IndexType attributeLocation, vtkIdType expectedTupleCount,
        QMap<QString, ArrayInfo> & arrayInfos) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            vtkDataArray * dataArray = attributes->GetArray(i);
            if (!dataArray)
                continue;

            // skip arrays that are marked as auxiliary
            vtkInformation * dataArrayInfo = dataArray->GetInformation();
            if (dataArrayInfo->Has(DataObject::ArrayIsAuxiliaryKey())
                && dataArrayInfo->Get(DataObject::ArrayIsAuxiliaryKey()))
                continue;

            QString name = QString::fromUtf8(dataArray->GetName());

            const vtkIdType tupleCount = dataArray->GetNumberOfTuples();
            if (expectedTupleCount > tupleCount)
            {
                qDebug() << "Not enough tuples in array" << name << ":" << tupleCount << ", expected" << expectedTupleCount << ", location" << attributeLocation << "(skipping)";
                continue;
            }
            else if (expectedTupleCount < tupleCount)
            {
                qDebug() << "Too many tuples in array" << name << ":" << tupleCount << ", expected" << expectedTupleCount << ", location" << attributeLocation << "(ignoring superfluous data)";
            }

            ArrayInfo & arrayInfo = arrayInfos[name];

            int lastNumComp = arrayInfo.numComponents;
            int currentNumComp = dataArray->GetNumberOfComponents();

            if (lastNumComp && lastNumComp != currentNumComp)
            {
                qDebug() << "Array named" << name << "found with different number of components (" << lastNumComp << "," << dataArray->GetNumberOfComponents() << ")";
                continue;
            }
            auto lastLocation = arrayInfo.attributeLocations.value(vis, IndexType::invalid);
            if (lastLocation != IndexType::invalid && lastLocation != attributeLocation)
            {
                qDebug() << "Array named" << name << "found in different attribute locations in the same data set";
                continue;
            }

            arrayInfo.numComponents = currentNumComp;
            arrayInfo.attributeLocations[vis] = attributeLocation;
        }
    };

    QList<AbstractVisualizedData *> supportedData;
    QMap<QString, ArrayInfo> arrayInfos;

    // list all available array names, check for same number of components
    for (AbstractVisualizedData * vis : visualizedData)
    {
        if (vis->contentType() == ContentType::Context2D)   // don't try to map colors to a plot
            continue;

        supportedData << vis;

        for (auto i = 0; i < vis->numberOfColorMappingInputs(); ++i)
        {
            vtkDataSet * dataSet = vis->colorMappingInputData(i);

            // in case of conflicts, prefer point over cell arrays (as they probably have a higher precision)
            checkAttributeArrays(vis, dataSet->GetPointData(), IndexType::points, dataSet->GetNumberOfPoints(), arrayInfos);
            checkAttributeArrays(vis, dataSet->GetCellData(), IndexType::cells, dataSet->GetNumberOfCells(), arrayInfos);
        }
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (auto it = arrayInfos.begin(); it != arrayInfos.end(); ++it)
    {
        const auto & arrayInfo = it.value();
        auto mapping = std::make_unique<AttributeArrayComponentMapping>(
            supportedData, 
            it.key(), 
            arrayInfo.numComponents,
            arrayInfo.attributeLocations);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

AttributeArrayComponentMapping::AttributeArrayComponentMapping(
    const QList<AbstractVisualizedData *> & visualizedData, const QString & dataArrayName, 
    int numDataComponents, const QMap<AbstractVisualizedData *, IndexType> & attributeLocations)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_dataArrayName{ dataArrayName }
    , m_attributeLocations(attributeLocations)
{
    assert(!visualizedData.isEmpty());

    m_isValid = true;
}

AttributeArrayComponentMapping::~AttributeArrayComponentMapping() = default;

QString AttributeArrayComponentMapping::name() const
{
    return m_dataArrayName;
}

QString AttributeArrayComponentMapping::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return m_dataArrayName;
}

IndexType AttributeArrayComponentMapping::scalarsAssociation(AbstractVisualizedData & vis) const
{
    return m_attributeLocations.value(&vis, IndexType::invalid);
}

vtkSmartPointer<vtkAlgorithm> AttributeArrayComponentMapping::createFilter(AbstractVisualizedData & visualizedData, int connection)
{
    const auto attributeLocation = m_attributeLocations.value(&visualizedData, IndexType::invalid);

    if (attributeLocation == IndexType::invalid)
    {
        auto filter = vtkSmartPointer<vtkPassThrough>::New();
        filter->SetInputConnection(visualizedData.colorMappingInput(connection));
        return filter;
    }

    auto filter = vtkSmartPointer<vtkAssignAttribute>::New();
    filter->SetInputConnection(visualizedData.colorMappingInput(connection));
    filter->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
        attributeLocation == IndexType::points ? vtkAssignAttribute::POINT_DATA : vtkAssignAttribute::CELL_DATA);
    return filter;
}

bool AttributeArrayComponentMapping::usesFilter() const
{
    return true;
}

void AttributeArrayComponentMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & abstractMapper)
{
    ColorMappingData::configureMapper(visualizedData, abstractMapper);

    const auto attributeLocation = m_attributeLocations.value(&visualizedData, IndexType::invalid);
    auto mapper = vtkMapper::SafeDownCast(&abstractMapper);

    if (mapper && attributeLocation != IndexType::invalid)
    {
        mapper->ScalarVisibilityOn();

        if (attributeLocation == IndexType::cells)
        {
            mapper->SetScalarModeToUseCellData();
        }
        else if (attributeLocation == IndexType::points)
        {
            mapper->SetScalarModeToUsePointData();
        }
        mapper->SelectColorArray(m_dataArrayName.toUtf8().data());
    }
    else if (mapper)
    {
        mapper->ScalarVisibilityOff();
    }
}

std::vector<ValueRange<>> AttributeArrayComponentMapping::updateBounds()
{
    const auto utf8Name = m_dataArrayName.toUtf8();

    auto bounds = decltype(updateBounds())(numDataComponents());

    for (auto c = 0; c < numDataComponents(); ++c)
    {
        decltype(bounds)::value_type totalRange;

        for (auto visualizedData : m_visualizedData)
        {
            auto attributeLocation = m_attributeLocations.value(visualizedData, IndexType::invalid);
            if (attributeLocation == IndexType::invalid)
            {
                continue;
            }

            for (int i = 0; i < visualizedData->numberOfColorMappingInputs(); ++i)
            {
                auto dataSet = visualizedData->colorMappingInputData(i);
                vtkDataArray * dataArray = 
                    attributeLocation == IndexType::cells ? dataSet->GetCellData()->GetArray(utf8Name.data())
                    : (attributeLocation == IndexType::points ? dataSet->GetPointData()->GetArray(utf8Name.data())
                    : nullptr);

                if (!dataArray)
                {
                    continue;
                }

                decltype(totalRange) range;
                dataArray->GetRange(range.data(), c);

                // ignore arrays with invalid data
                if (range.isEmpty())
                {
                    continue;
                }

                totalRange.add(range);
            }
        }

        if (totalRange.isEmpty())    // invalid data in all arrays on this component
        {
            totalRange[0] = totalRange[1] = 0.0;
        }

        bounds[c] = totalRange;
    }

    return bounds;
}
