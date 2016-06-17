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


namespace
{
    const QString s_name = "direct image colors";
}

const bool DirectImageColors::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<ColorMappingData>> DirectImageColors::newInstances(const QList<AbstractVisualizedData *> & visualizedData)
{
    QMultiMap<QString, int> arrayLocs;

    auto checkAddAttributeArrays = [&arrayLocs] (vtkDataSetAttributes * attributes, int attributeLocation) -> void
    {
        for (auto i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            vtkUnsignedCharArray * colors = vtkUnsignedCharArray::SafeDownCast(attributes->GetArray(i));
            if (!colors || colors->GetNumberOfComponents() != 3)
                continue;

            // skip arrays that are marked as auxiliary
            vtkInformation * arrayInfo = colors->GetInformation();
            if (arrayInfo->Has(DataObject::ArrayIsAuxiliaryKey())
                && arrayInfo->Get(DataObject::ArrayIsAuxiliaryKey()))
                continue;

            QString name = QString::fromUtf8(colors->GetName());

            arrayLocs.insert(name, attributeLocation);
        }
    };

    QList<AbstractVisualizedData *> supportedData;

    // list all available array names, check for same number of components
    for (AbstractVisualizedData * vis : visualizedData)
    {
        if (vis->contentType() == ContentType::Context2D)   // don't try to map colors to a plot
            continue;

        supportedData << vis;

        for (auto i = 0; i < vis->numberOfColorMappingInputs(); ++i)
        {
            vtkDataSet * dataSet = vis->colorMappingInputData(i);

            checkAddAttributeArrays(dataSet->GetCellData(), vtkAssignAttribute::CELL_DATA);
            checkAddAttributeArrays(dataSet->GetPointData(), vtkAssignAttribute::POINT_DATA);
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

DirectImageColors::DirectImageColors(const QList<AbstractVisualizedData *> & visualizedData, QString dataArrayName, int attributeLocation)
    : ColorMappingData(visualizedData)
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

QString DirectImageColors::scalarsName() const
{
    return m_dataArrayName;
}

vtkSmartPointer<vtkAlgorithm> DirectImageColors::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    auto filter = vtkSmartPointer<vtkAssignAttribute>::New();
    filter->SetInputConnection(visualizedData->colorMappingInput(connection));
    filter->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
        m_attributeLocation);

    return filter;
}

bool DirectImageColors::usesFilter() const
{
    return true;
}

void DirectImageColors::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);

    if (auto m = vtkMapper::SafeDownCast(mapper))
    {
        m->ScalarVisibilityOn();
        if (m_attributeLocation == vtkAssignAttribute::CELL_DATA)
            m->SetScalarModeToUseCellData();
        else if (m_attributeLocation == vtkAssignAttribute::POINT_DATA)
            m->SetScalarModeToUsePointData();
        m->SelectColorArray(m_dataArrayName.toUtf8().data());

        m->SetColorModeToDirectScalars();
    }
}

QMap<int, QPair<double, double>> DirectImageColors::updateBounds()
{
    // value range is 0..0xFF, but is not supposed to be configured in the ui
    return{
        { 0, { 0, 0 } }
    };
}
