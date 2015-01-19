#include "VectorMagnitudeColorMapping.h"

#include <cassert>

#include <QSet>

#include <vtkInformation.h>

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>

#include <vtkVectorNorm.h>
#include <vtkAssignAttribute.h>

#include <vtkMapper.h>

#include <core/vtkhelper.h>
#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>
#include <core/color_mapping/ColorMappingRegistry.h>


namespace
{
    const QString s_name = "vertex magnitude";
}

const bool VectorMagnitudeColorMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<ColorMappingData *> VectorMagnitudeColorMapping::newInstances(const QList<AbstractVisualizedData*> & visualizedData)
{
    auto checkAddAttributeArrays = [] (vtkDataSetAttributes * attributes, QSet<QString> & arrayNames) -> bool
    {
        bool hasVectors = false;

        for (vtkIdType i = 0; i < attributes->GetNumberOfArrays(); ++i)
        {
            vtkDataArray * dataArray = attributes->GetArray(i);
            if (!dataArray)
                continue;

            // skip arrays that are marked as auxiliary
            vtkInformation * arrayInfo = dataArray->GetInformation();
            if (arrayInfo->Has(DataObject::ArrayIsAuxiliaryKey())
                && arrayInfo->Get(DataObject::ArrayIsAuxiliaryKey()))
                continue;

            // vtkVectorNorm only supports 3-component vectors
            if (dataArray->GetNumberOfComponents() != 3)
                continue;

            arrayNames << QString::fromUtf8(dataArray->GetName());
            hasVectors = true;
        }

        return hasVectors;
    };

    QSet<QString> cellArrays, pointArrays;
    QMap<AbstractVisualizedData *, bool> hasCellVectorData, hasPointVectorData;

    for (AbstractVisualizedData * vis : visualizedData)
    {
        vtkDataSet * dataSet = vis->dataObject()->processedDataSet();

        hasCellVectorData[vis] =
            checkAddAttributeArrays(dataSet->GetCellData(), cellArrays);
        hasPointVectorData[vis] =
            checkAddAttributeArrays(dataSet->GetPointData(), pointArrays);
    }

    QList<AbstractVisualizedData *> withCellVectors, withPointVectors;

    for (auto it = hasCellVectorData.begin(); it != hasCellVectorData.end(); ++it)
        if (it.value())
            withCellVectors << it.key();

    for (auto it = hasPointVectorData.begin(); it != hasPointVectorData.end(); ++it)
        if (it.value())
            withPointVectors << it.key();

    QList<VectorMagnitudeColorMapping *> unchecked;

    for (const QString & arrayName : cellArrays)
        unchecked << new VectorMagnitudeColorMapping(withCellVectors, arrayName, vtkAssignAttribute::CELL_DATA);
    for (const QString & arrayName : pointArrays)
        unchecked << new VectorMagnitudeColorMapping(withPointVectors, arrayName, vtkAssignAttribute::POINT_DATA);

    QList<ColorMappingData *> instances;
    for (VectorMagnitudeColorMapping * mapping : unchecked)
    {
        if (mapping->isValid())
        {
            mapping->initialize();
            instances << mapping;
        }
        else
            delete mapping;
    }

    return instances;
}

VectorMagnitudeColorMapping::VectorMagnitudeColorMapping(
    const QList<AbstractVisualizedData *> & visualizedData,
    const QString & dataArrayName, int attributeLocation)
    : ColorMappingData(visualizedData)
    , m_attributeLocation(attributeLocation)
    , m_dataArrayName(dataArrayName)
{
    QByteArray utf8Name = m_dataArrayName.toUtf8();

    // establish pipeline here, so that we have valid data whenever updateBounds() requests norm outputs

    for (AbstractVisualizedData * vis : visualizedData)
    {
        VTK_CREATE(vtkAssignAttribute, activeVectors);
        activeVectors->Assign(utf8Name.data(), vtkDataSetAttributes::VECTORS, m_attributeLocation);
        activeVectors->SetInputConnection(vis->dataObject()->processedOutputPort());

        VTK_CREATE(vtkVectorNorm, norm);
        if (attributeLocation == vtkAssignAttribute::CELL_DATA)
            norm->SetAttributeModeToUseCellData();
        else if (attributeLocation == vtkAssignAttribute::POINT_DATA)
            norm->SetAttributeModeToUsePointData();

        norm->SetInputConnection(activeVectors->GetOutputPort());

        m_vectorNorms.insert(vis, norm);
    }

    m_isValid = true;
}

VectorMagnitudeColorMapping::~VectorMagnitudeColorMapping() = default;

QString VectorMagnitudeColorMapping::name() const
{
    return m_dataArrayName + " Magnitude";
}

vtkAlgorithm * VectorMagnitudeColorMapping::createFilter(AbstractVisualizedData * visualizedData)
{
    /** vtkVectorNorm sets norm array as current scalars; it doesn't set a name */
    vtkVectorNorm * norm = m_vectorNorms.value(visualizedData);
    assert(norm);

    // the reference count of the output is expected to be +1 ("create..")
    norm->Register(nullptr);

    return norm;
}

bool VectorMagnitudeColorMapping::usesFilter() const
{
    return true;
}

void VectorMagnitudeColorMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper)
{
    ColorMappingData::configureMapper(visualizedData, mapper);

    mapper->ScalarVisibilityOn();

    if (m_attributeLocation == vtkAssignAttribute::CELL_DATA)
        mapper->SetScalarModeToUseCellData();
    else if (m_attributeLocation == vtkAssignAttribute::POINT_DATA)
        mapper->SetScalarModeToUsePointData();
}

void VectorMagnitudeColorMapping::updateBounds()
{
    double totalRange[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };

    for (vtkVectorNorm * norm : m_vectorNorms.values())
    {
        norm->Update();
        vtkDataSet * dataSet = norm->GetOutput();
        vtkDataArray * normData = nullptr;
        if (m_attributeLocation == vtkAssignAttribute::CELL_DATA)
            normData = dataSet->GetCellData()->GetScalars();
        else if (m_attributeLocation == vtkAssignAttribute::POINT_DATA)
            normData = dataSet->GetPointData()->GetScalars();
        assert(normData);

        double range[2];
        normData->GetRange(range);
        totalRange[0] = std::min(totalRange[0], range[0]);
        totalRange[1] = std::max(totalRange[1], range[1]);
    }

    setDataMinMaxValue(totalRange, 0);
}
