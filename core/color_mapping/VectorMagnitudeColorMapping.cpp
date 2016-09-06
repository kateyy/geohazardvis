#include "VectorMagnitudeColorMapping.h"

#include <cassert>

#include <QVector>

#include <vtkInformation.h>

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>

#include <vtkVectorNorm.h>
#include <vtkAssignAttribute.h>

#include <vtkMapper.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/utility/DataExtent.h>


namespace
{
    const QString s_name = "vector magnitude";
}

const bool VectorMagnitudeColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> VectorMagnitudeColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
{
    auto checkAddAttributeArrays = [] (AbstractVisualizedData * vis, vtkDataSetAttributes * attributes, QMap<QString, QList<AbstractVisualizedData *>> & arrayNames) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            auto dataArray = attributes->GetArray(i);
            if (!dataArray)
            {
                continue;
            }

            // skip arrays that are marked as auxiliary
            auto arrayInfo = dataArray->GetInformation();
            if (arrayInfo->Has(DataObject::ArrayIsAuxiliaryKey())
                && arrayInfo->Get(DataObject::ArrayIsAuxiliaryKey()))
            {
                continue;
            }

            // vtkVectorNorm only supports 3-component vectors
            if (dataArray->GetNumberOfComponents() != 3)
            {
                continue;
            }

            // store a list of relevant objects for each kind of attribute data
            arrayNames[QString::fromUtf8(dataArray->GetName())] << vis;
        }
    };

    QMap<QString, QList<AbstractVisualizedData *>> cellArrays, pointArrays;

    for (auto vis : visualizedData)
    {
        for (int i = 0; i < vis->numberOfColorMappingInputs(); ++i)
        {
            auto dataSet = vis->colorMappingInputData(i);

            checkAddAttributeArrays(vis, dataSet->GetCellData(), cellArrays);
            checkAddAttributeArrays(vis, dataSet->GetPointData(), pointArrays);
        }
    }

    std::vector<std::unique_ptr<VectorMagnitudeColorMapping>> unchecked;

    for (auto it = cellArrays.begin(); it != cellArrays.end(); ++it)
    {
        unchecked.push_back(
            std::make_unique<VectorMagnitudeColorMapping>(it.value(), it.key(), IndexType::cells));
    }
    for (auto it = pointArrays.begin(); it != pointArrays.end(); ++it)
    {
        unchecked.push_back(
            std::make_unique<VectorMagnitudeColorMapping>(it.value(), it.key(), IndexType::points));
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
    const QList<AbstractVisualizedData *> & visualizedData,
    const QString & dataArrayName, IndexType attributeLocation)
    : ColorMappingData(visualizedData)
    , m_attributeLocation{ attributeLocation }
    , m_dataArrayName{ dataArrayName }
    , m_magnitudeArrayName{ dataArrayName + " Magnitude" }
{
    const auto utf8Name = m_dataArrayName.toUtf8();

    // establish pipeline here, so that we have valid data whenever updateBounds() requests norm outputs

    const auto assignLocation = attributeLocation == IndexType::points
        ? vtkAssignAttribute::POINT_DATA
        : vtkAssignAttribute::CELL_DATA;
    const auto renameLocation = attributeLocation == IndexType::points
        ? ArrayChangeInformationFilter::AttributeLocations::POINT_DATA
        : ArrayChangeInformationFilter::AttributeLocations::CELL_DATA;

    for (auto vis : visualizedData)
    {
        QVector<vtkSmartPointer<vtkVectorNorm>> norms;

        for (int i = 0; i < vis->numberOfColorMappingInputs(); ++i)
        {
            auto activeVectors = vtkSmartPointer<vtkAssignAttribute>::New();
            activeVectors->Assign(utf8Name.data(), vtkDataSetAttributes::VECTORS, assignLocation);
            activeVectors->SetInputConnection(vis->colorMappingInput(i));

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

            norms << norm;
        }

        m_vectorNorms.insert(vis, norms);
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

vtkSmartPointer<vtkAlgorithm> VectorMagnitudeColorMapping::createFilter(AbstractVisualizedData & visualizedData, int connection)
{
    /** vtkVectorNorm sets norm array as current scalars; it doesn't set a name */
    auto & norms = m_vectorNorms.value(&visualizedData);

    assert(norms.size() > connection);
    auto norm = norms[connection];
    assert(norm);

    return norm;
}

bool VectorMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void VectorMagnitudeColorMapping::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->ScalarVisibilityOn();

        if (m_attributeLocation == IndexType::cells)
        {
            m->SetScalarModeToUseCellData();
        }
        else if (m_attributeLocation == IndexType::points)
        {
            m->SetScalarModeToUsePointData();
        }
    }
}

std::vector<ValueRange<>> VectorMagnitudeColorMapping::updateBounds()
{
    decltype(updateBounds())::value_type totalRange;

    for (auto norms : m_vectorNorms.values())
    {
        for (auto norm : norms)
        {
            norm->Update();
            auto dataSet = norm->GetOutput();
            vtkDataArray * normData = nullptr;
            if (m_attributeLocation == IndexType::cells)
            {
                normData = dataSet->GetCellData()->GetScalars();
            }
            else if (m_attributeLocation == IndexType::points)
            {
                normData = dataSet->GetPointData()->GetScalars();
            }
            assert(normData);

            decltype(totalRange) range;
            normData->GetRange(range.data());
            totalRange.add(range);
        }
    }

    return{ totalRange };
}
