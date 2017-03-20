#include <core/color_mapping/DirectImageColors.h>

#include <map>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>

#include <core/AbstractVisualizedData.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/types_utils.h>


namespace
{
    const QString s_name = "direct image colors";
}

const bool DirectImageColors::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> DirectImageColors::newInstances(const std::vector<AbstractVisualizedData *> & visualizedData)
{
    std::multimap<QString, IndexType> arrayLocs;

    auto checkAddAttributeArrays = [&arrayLocs] (vtkDataSetAttributes * attributes, IndexType attributeLocation) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            auto colors = vtkUnsignedCharArray::FastDownCast(attributes->GetAbstractArray(i));
            if (!colors || colors->GetNumberOfComponents() != 3)
            {
                continue;
            }

            // skip arrays that are marked as auxiliary
            auto arrayInfo = colors->GetInformation();
            if (arrayInfo->Has(DataObject::ARRAY_IS_AUXILIARY())
                && arrayInfo->Get(DataObject::ARRAY_IS_AUXILIARY()))
            {
                continue;
            }

            arrayLocs.emplace(QString::fromUtf8(colors->GetName()), attributeLocation);
        }
    };

    std::vector<AbstractVisualizedData *> supportedData;

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

            checkAddAttributeArrays(dataSet->GetCellData(), IndexType::cells);
            checkAddAttributeArrays(dataSet->GetPointData(), IndexType::points);
        }
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (const auto & loc : arrayLocs)
    {
        auto mapping = std::make_unique<DirectImageColors>(supportedData, loc.first, loc.second);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

DirectImageColors::DirectImageColors(const std::vector<AbstractVisualizedData *> & visualizedData,
    const QString & dataArrayName, IndexType attributeLocation)
    : ColorMappingData(visualizedData, 1, false)
    , m_attributeLocation{ attributeLocation }
    , m_dataArrayName{ dataArrayName }
{
    assert(!visualizedData.empty());

    m_isValid = true;
}

DirectImageColors::~DirectImageColors() = default;

QString DirectImageColors::name() const
{
    return m_dataArrayName + " (direct colors)";
}

QString DirectImageColors::scalarsName(AbstractVisualizedData & /*vis*/) const
{
    return m_dataArrayName;
}

IndexType DirectImageColors::scalarsAssociation(AbstractVisualizedData & /*vis*/) const
{
    return m_attributeLocation;
}

vtkSmartPointer<vtkAlgorithm> DirectImageColors::createFilter(AbstractVisualizedData & visualizedData, unsigned int port)
{
    auto filter = vtkSmartPointer<vtkAssignAttribute>::New();
    filter->SetInputConnection(visualizedData.processedOutputPort(port));
    filter->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
        IndexType_util(m_attributeLocation).toVtkAssignAttribute_AttributeLocation());

    return filter;
}

bool DirectImageColors::usesFilter() const
{
    return true;
}

void DirectImageColors::configureMapper(
    AbstractVisualizedData & visualizedData,
    vtkAbstractMapper & mapper,
    unsigned int port)
{
    ColorMappingData::configureMapper(visualizedData, mapper, port);

    if (auto m = vtkMapper::SafeDownCast(&mapper))
    {
        m->ScalarVisibilityOn();
        m->SetColorModeToDirectScalars();
        if (m_attributeLocation == IndexType::cells)
        {
            m->SetScalarModeToUseCellFieldData();
        }
        else if (m_attributeLocation == IndexType::points)
        {
            m->SetScalarModeToUsePointFieldData();
        }
        m->SelectColorArray(m_dataArrayName.toUtf8().data());
    }
}

std::vector<ValueRange<>> DirectImageColors::updateBounds()
{
    // value range is 0..0xFF, but is not supposed to be configured in the ui
    return{ decltype(updateBounds())::value_type({ 0, 0 }) };
}
