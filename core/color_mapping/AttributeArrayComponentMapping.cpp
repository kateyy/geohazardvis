#include "AttributeArrayComponentMapping.h"

#include <algorithm>
#include <cassert>

#include <QDebug>

#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkMapper.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>

#include <core/AbstractVisualizedData.h>
#include <core/CoordinateSystems.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/AttributeArrayModifiedListener.h>
#include <core/utility/DataExtent.h>
#include <core/utility/types_utils.h>


namespace
{
const QString s_name = "attribute array component";
}

const bool AttributeArrayComponentMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> AttributeArrayComponentMapping::newInstances(
    const std::vector<AbstractVisualizedData *> & visualizedData)
{
    struct ArrayInfo
    {
        explicit ArrayInfo(int comp = 0)
            : numComponents{ comp }
            , isTemporal{ false }
        {
        }
        int numComponents;
        bool isTemporal;
        std::map<AbstractVisualizedData *, IndexType> attributeLocations;
    };

    auto checkAttributeArrays = [] (
        AbstractVisualizedData * vis, vtkDataSetAttributes * attributes,
        IndexType attributeLocation, vtkIdType expectedTupleCount,
        std::map<QString, ArrayInfo> & arrayInfos) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            auto dataArray = attributes->GetArray(i);
            if (!dataArray)
            {
                continue;
            }

            // skip arrays that are marked as auxiliary
            auto & dataArrayInfo = *dataArray->GetInformation();
            if (dataArrayInfo.Has(DataObject::ARRAY_IS_AUXILIARY())
                && dataArrayInfo.Get(DataObject::ARRAY_IS_AUXILIARY()))
            {
                continue;
            }
            // skip point coordinates stored in point data
            if (CoordinateSystemSpecification::fromInformation(dataArrayInfo).isValid())
            {
                continue;
            }

            const auto name = QString::fromUtf8(dataArray->GetName());

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

            auto & arrayInfo = arrayInfos[name];

            const int lastNumComp = arrayInfo.numComponents;
            const int currentNumComp = dataArray->GetNumberOfComponents();

            if (lastNumComp && lastNumComp != currentNumComp)
            {
                qDebug() << "Array named" << name << "found with different number of components (" << lastNumComp << "," << dataArray->GetNumberOfComponents() << ")";
                continue;
            }
            const auto lastLocationIt = arrayInfo.attributeLocations.find(vis);
            if (lastLocationIt != arrayInfo.attributeLocations.end() && lastLocationIt->second != attributeLocation)
            {
                qDebug() << "Array named" << name << "found in different attribute locations in the same data set";
                continue;
            }

            arrayInfo.isTemporal = arrayInfo.isTemporal ||
                (0 != dataArray->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()));

            arrayInfo.numComponents = currentNumComp;
            arrayInfo.attributeLocations[vis] = attributeLocation;
        }
    };

    std::vector<AbstractVisualizedData *> supportedData;
    std::map<QString, ArrayInfo> arrayInfos;

    // list all available array names, check for same number of components
    for (auto vis : visualizedData)
    {
        if (vis->contentType() == ContentType::Context2D)   // don't try to map colors to a plot
        {
            continue;
        }

        supportedData.emplace_back(vis);

        for (unsigned int i = 0; i < vis->numberOfOutputPorts(); ++i)
        {
            auto dataSet = vis->processedOutputDataSet(i);

            // in case of conflicts, prefer point over cell arrays (as they probably have a higher precision)
            checkAttributeArrays(vis, dataSet->GetPointData(), IndexType::points, dataSet->GetNumberOfPoints(), arrayInfos);
            checkAttributeArrays(vis, dataSet->GetCellData(), IndexType::cells, dataSet->GetNumberOfCells(), arrayInfos);
        }
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (const auto & pair : arrayInfos)
    {
        const auto & arrayInfo = pair.second;
        auto mapping = std::make_unique<AttributeArrayComponentMapping>(
            supportedData,
            pair.first,
            arrayInfo.numComponents,
            arrayInfo.isTemporal,
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
    const std::vector<AbstractVisualizedData *> & visualizedData,
    const QString & dataArrayName,
    int numDataComponents,
    bool isTemporalAttribute,
    const std::map<AbstractVisualizedData *, IndexType> & attributeLocations)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_dataArrayName{ dataArrayName }
    , m_isTemporalAttribute{ isTemporalAttribute }
    , m_attributeLocations(attributeLocations)
{
    assert(!visualizedData.empty());

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
    const auto it = m_attributeLocations.find(&vis);
    return it != m_attributeLocations.end()
        ? it->second
        : IndexType::invalid;
}

bool AttributeArrayComponentMapping::isTemporalAttribute() const
{
    return m_isTemporalAttribute;
}

vtkSmartPointer<vtkAlgorithm> AttributeArrayComponentMapping::createFilter(AbstractVisualizedData & visualizedData, unsigned int port)
{
    const auto visIt = m_filters.emplace(&visualizedData, decltype(m_filters)::mapped_type()).first;
    auto & filter = visIt->second.emplace(port, decltype(visIt->second)::mapped_type()).first->second;

    if (filter)
    {
        return filter;
    }

    const auto attributeLocation = scalarsAssociation(visualizedData);

    if (attributeLocation == IndexType::invalid)
    {
        filter = vtkSmartPointer<vtkPassThrough>::New();
        filter->SetInputConnection(visualizedData.processedOutputPort(port));
        return filter;
    }

    auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
    assign->SetInputConnection(visualizedData.processedOutputPort(port));
    assign->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
        IndexType_util(attributeLocation).toVtkAssignAttribute_AttributeLocation());

    auto modifiedListener = vtkSmartPointer<AttributeArrayModifiedListener>::New();
    modifiedListener->SetInputConnection(assign->GetOutputPort());
    modifiedListener->SetAttributeLocation(attributeLocation);

    connect(modifiedListener, &AttributeArrayModifiedListener::attributeModified,
        this, &AttributeArrayComponentMapping::forceUpdateBoundsLocked);

    filter = modifiedListener;

    return filter;
}

bool AttributeArrayComponentMapping::usesFilter() const
{
    return true;
}

void AttributeArrayComponentMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & abstractMapper, unsigned int port)
{
    ColorMappingData::configureMapper(visualizedData, abstractMapper, port);

    const auto attributeLocation = scalarsAssociation(visualizedData);
    auto mapper = vtkMapper::SafeDownCast(&abstractMapper);

    if (mapper && attributeLocation != IndexType::invalid)
    {
        mapper->ScalarVisibilityOn();
        mapper->SetColorModeToMapScalars();

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
            const auto attributeLocation = IndexType_util(scalarsAssociation(*visualizedData));
            if (attributeLocation == IndexType::invalid)
            {
                continue;
            }

            for (unsigned int i = 0; i < visualizedData->numberOfOutputPorts(); ++i)
            {
                auto dataArray = attributeLocation.extractArray(
                    visualizedData->processedOutputDataSet(i), utf8Name.data());
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
