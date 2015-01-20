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
    auto checkAddAttributeArrays = [] (AbstractVisualizedData * vis, vtkDataSetAttributes * attributes, QMap<QString, QList<AbstractVisualizedData *>> & arrayNames) -> void
    {
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

            // store a list of relevant objects for each kind of attribute data
            arrayNames[QString::fromUtf8(dataArray->GetName())] << vis;
        }
    };

    QMap<QString, QList<AbstractVisualizedData *>> cellArrays, pointArrays;

    for (AbstractVisualizedData * vis : visualizedData)
    {
        vtkDataSet * dataSet = vis->dataObject()->processedDataSet();

        checkAddAttributeArrays(vis, dataSet->GetCellData(), cellArrays);
        checkAddAttributeArrays(vis, dataSet->GetPointData(), pointArrays);
    }

    QList<VectorMagnitudeColorMapping *> unchecked;

    for (auto it = cellArrays.begin(); it != cellArrays.end(); ++it)
        unchecked << new VectorMagnitudeColorMapping(it.value(), it.key(), vtkAssignAttribute::CELL_DATA);
    for (auto it = pointArrays.begin(); it != pointArrays.end(); ++it)
        unchecked << new VectorMagnitudeColorMapping(it.value(), it.key(), vtkAssignAttribute::POINT_DATA);

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

QMap<vtkIdType, QPair<double, double>> VectorMagnitudeColorMapping::updateBounds()
{
    double totalMin = std::numeric_limits<double>::max();
    double totalMax = std::numeric_limits<double>::lowest();

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
        totalMin = std::min(totalMin, range[0]);
        totalMax = std::max(totalMax, range[1]);
    }

    return{ { 0, { totalMin, totalMax } } };
}
