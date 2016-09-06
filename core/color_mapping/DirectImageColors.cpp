#include <core/color_mapping/DirectImageColors.h>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/color_mapping/ColorMappingRegistry.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/DataExtent.h>


namespace
{
    const QString s_name = "direct image colors";
}

const bool DirectImageColors::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> DirectImageColors::newInstances(const QList<AbstractVisualizedData *> & visualizedData)
{
    QMultiMap<QString, IndexType> arrayLocs;

    auto checkAddAttributeArrays = [&arrayLocs] (vtkDataSetAttributes * attributes, IndexType attributeLocation) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            auto colors = vtkUnsignedCharArray::SafeDownCast(attributes->GetArray(i));
            if (!colors || colors->GetNumberOfComponents() != 3)
            {
                continue;
            }

            // skip arrays that are marked as auxiliary
            auto arrayInfo = colors->GetInformation();
            if (arrayInfo->Has(DataObject::ArrayIsAuxiliaryKey())
                && arrayInfo->Get(DataObject::ArrayIsAuxiliaryKey()))
            {
                continue;
            }

            arrayLocs.insert(QString::fromUtf8(colors->GetName()), attributeLocation);
        }
    };

    QList<AbstractVisualizedData *> supportedData;

    // list all available array names, check for same number of components
    for (auto vis : visualizedData)
    {
        if (vis->contentType() == ContentType::Context2D)   // don't try to map colors to a plot
        {
            continue;
        }

        supportedData << vis;

        for (auto i = 0; i < vis->numberOfColorMappingInputs(); ++i)
        {
            auto dataSet = vis->colorMappingInputData(i);

            checkAddAttributeArrays(dataSet->GetCellData(), IndexType::cells);
            checkAddAttributeArrays(dataSet->GetPointData(), IndexType::points);
        }
    }

    std::vector<std::unique_ptr<ColorMappingData>> instances;
    for (auto it = arrayLocs.begin(); it != arrayLocs.end(); ++it)
    {
        auto mapping = std::make_unique<DirectImageColors>(supportedData, it.key(), it.value());
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

DirectImageColors::DirectImageColors(const QList<AbstractVisualizedData *> & visualizedData,
    const QString & dataArrayName, IndexType attributeLocation)
    : ColorMappingData(visualizedData, 1, false)
    , m_attributeLocation{ attributeLocation }
    , m_dataArrayName{ dataArrayName }
{
    assert(!visualizedData.isEmpty());

    m_isValid = true;
}

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

vtkSmartPointer<vtkAlgorithm> DirectImageColors::createFilter(AbstractVisualizedData & visualizedData, int connection)
{
    auto filter = vtkSmartPointer<vtkAssignAttribute>::New();
    filter->SetInputConnection(visualizedData.colorMappingInput(connection));
    filter->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
        m_attributeLocation == IndexType::points ? vtkAssignAttribute::POINT_DATA : vtkAssignAttribute::CELL_DATA);

    return filter;
}

bool DirectImageColors::usesFilter() const
{
    return true;
}

void DirectImageColors::configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper)
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
        m->SelectColorArray(m_dataArrayName.toUtf8().data());

        m->SetColorModeToDirectScalars();
    }
}

std::vector<ValueRange<>> DirectImageColors::updateBounds()
{
    // value range is 0..0xFF, but is not supposed to be configured in the ui
    return{ decltype(updateBounds())::value_type({ 0, 0 }) };
}
