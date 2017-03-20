#include "VectorMagnitudeColorMapping.h"

#include <cassert>

#include <QDebug>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkMapper.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>
#include <vtkVectorNorm.h>

#include <core/AbstractVisualizedData.h>
#include <core/CoordinateSystems.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/utility/DataExtent.h>
#include <core/utility/types_utils.h>


namespace
{
    const QString s_name = "vector magnitude";
}

const bool VectorMagnitudeColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> VectorMagnitudeColorMapping::newInstances(const std::vector<AbstractVisualizedData*> & visualizedData)
{
    auto checkAddAttributeArrays = [] (AbstractVisualizedData * vis, vtkDataSetAttributes * attributes, std::map<QString, std::vector<AbstractVisualizedData *>> & arrayNames) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            auto dataArray = attributes->GetArray(i);
            if (!dataArray)
            {
                continue;
            }

            // skip arrays that are marked as auxiliary
            auto & arrayInfo = *dataArray->GetInformation();
            if (arrayInfo.Has(DataObject::ARRAY_IS_AUXILIARY())
                && arrayInfo.Get(DataObject::ARRAY_IS_AUXILIARY()))
            {
                continue;
            }
            // skip point coordinates stored in point data
            if (CoordinateSystemSpecification::fromInformation(arrayInfo).isValid())
            {
                continue;
            }

            // vtkVectorNorm only supports 3-component vectors
            if (dataArray->GetNumberOfComponents() != 3)
            {
                continue;
            }

            // store a list of relevant objects for each kind of attribute data
            arrayNames[QString::fromUtf8(dataArray->GetName())].emplace_back(vis);
        }
    };

    std::map<QString, std::vector<AbstractVisualizedData *>> cellArrays, pointArrays;

    for (auto vis : visualizedData)
    {
        if (vis->contentType() != ContentType::Rendered2D && vis->contentType() != ContentType::Rendered3D)
        {
            continue;
        }

        for (unsigned int i = 0; i < vis->numberOfOutputPorts(); ++i)
        {
            auto dataSet = vis->processedOutputDataSet(i);

            checkAddAttributeArrays(vis, dataSet->GetCellData(), cellArrays);
            checkAddAttributeArrays(vis, dataSet->GetPointData(), pointArrays);
        }
    }

    std::vector<std::unique_ptr<VectorMagnitudeColorMapping>> unchecked;

    for (const auto & it : cellArrays)
    {
        unchecked.push_back(
            std::make_unique<VectorMagnitudeColorMapping>(it.second, it.first, IndexType::cells));
    }
    for (const auto & it : pointArrays)
    {
        unchecked.push_back(
            std::make_unique<VectorMagnitudeColorMapping>(it.second, it.first, IndexType::points));
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (auto & mapping : unchecked)
    {
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

VectorMagnitudeColorMapping::VectorMagnitudeColorMapping(
    const std::vector<AbstractVisualizedData *> & visualizedData,
    const QString & dataArrayName, IndexType attributeLocation)
    : ColorMappingData(visualizedData)
    , m_attributeLocation{ attributeLocation }
    , m_dataArrayName{ dataArrayName }
    , m_magnitudeArrayName{ dataArrayName + " Magnitude" }
{
    const auto utf8Name = m_dataArrayName.toUtf8();
    const auto utf8MagnitudeName = m_magnitudeArrayName.toUtf8();

    // establish pipeline here, so that we have valid data whenever updateBounds() requests norm outputs

    const auto assignLocation = IndexType_util(attributeLocation).toVtkAssignAttribute_AttributeLocation();
    const auto renameLocation = IndexType_util(attributeLocation);

    for (auto vis : visualizedData)
    {
        std::vector<vtkSmartPointer<vtkAlgorithm>> filters;

        for (unsigned int i = 0; i < vis->numberOfOutputPorts(); ++i)
        {
            auto activeVectors = vtkSmartPointer<vtkAssignAttribute>::New();
            activeVectors->Assign(utf8Name.data(), vtkDataSetAttributes::VECTORS, assignLocation);
            activeVectors->SetInputConnection(vis->processedOutputPort(i));

            auto norm = vtkSmartPointer<vtkVectorNorm>::New();
            if (attributeLocation == IndexType::cells)
            {
                norm->SetAttributeModeToUseCellData();
            }
            else if (attributeLocation == IndexType::points)
            {
                norm->SetAttributeModeToUsePointData();
            }

            norm->SetInputConnection(activeVectors->GetOutputPort());

            auto setMagnitudeName = vtkSmartPointer<ArrayChangeInformationFilter>::New();
            setMagnitudeName->SetAttributeLocation(renameLocation);
            setMagnitudeName->SetAttributeType(vtkDataSetAttributes::SCALARS);
            setMagnitudeName->EnableRenameOn();
            setMagnitudeName->SetArrayName(utf8MagnitudeName.data());
            setMagnitudeName->SetInputConnection(norm->GetOutputPort());

            filters.emplace_back(setMagnitudeName);
        }

        m_filters.emplace(vis, filters);
    }

    m_isValid = true;
}

VectorMagnitudeColorMapping::~VectorMagnitudeColorMapping() = default;

QString VectorMagnitudeColorMapping::name() const
{
    return m_magnitudeArrayName;
}

QString VectorMagnitudeColorMapping::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return m_magnitudeArrayName;
}

IndexType VectorMagnitudeColorMapping::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return m_attributeLocation;
}

vtkSmartPointer<vtkAlgorithm> VectorMagnitudeColorMapping::createFilter(AbstractVisualizedData & visualizedData, unsigned int port)
{
    /** vtkVectorNorm sets norm array as current scalars; it doesn't set a name */
    const auto filtersIt = m_filters.find(&visualizedData);
    if (filtersIt == m_filters.end())
    {
        auto filter = vtkSmartPointer<vtkPassThrough>::New();
        filter->SetInputConnection(visualizedData.processedOutputPort(port));
        return filter;
    }

    assert(filtersIt->second.size() > port);
    auto & norm = filtersIt->second[port];
    assert(norm);

    return norm;
}

bool VectorMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void VectorMagnitudeColorMapping::configureMapper(
    AbstractVisualizedData & visualizedData,
    vtkAbstractMapper & mapper,
    unsigned int port)
{
    ColorMappingData::configureMapper(visualizedData, mapper, port);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->ScalarVisibilityOn();
        m->SetColorModeToMapScalars();

        if (m_attributeLocation == IndexType::cells)
        {
            m->SetScalarModeToUseCellFieldData();
        }
        else if (m_attributeLocation == IndexType::points)
        {
            m->SetScalarModeToUsePointFieldData();
        }

        m->SelectColorArray(m_magnitudeArrayName.toUtf8().data());
    }
}

std::vector<ValueRange<>> VectorMagnitudeColorMapping::updateBounds()
{
    decltype(updateBounds())::value_type totalRange;

    for (const auto & filtersIt : m_filters)
    {
        for (const auto & filter : filtersIt.second)
        {
            filter->Update();
            auto dataSet = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));
            auto attributes = IndexType_util(m_attributeLocation).extractAttributes(dataSet);
            auto normData = attributes ? attributes->GetScalars() : nullptr;
            if (!normData)
            {
                qDebug() << "Missing vector norm array in " + filtersIt.first->dataObject().name();
                continue;
            }

            decltype(totalRange) range;
            normData->GetRange(range.data());
            totalRange.add(range);
        }
    }

    return{ totalRange };
}
