#include "AttributeArrayComponentMapping.h"

#include <algorithm>
#include <cassert>
#include <limits>

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


namespace
{
const QString s_name = "attribute array component";
}

const bool AttributeArrayComponentMapping::s_isRegistered = ColorMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<ColorMappingData *> AttributeArrayComponentMapping::newInstances(const QList<AbstractVisualizedData *> & visualizedData)
{
    struct ArrayInfo
    {
        ArrayInfo(int comp = 0)
            : numComponents(comp)
        {
        }
        int numComponents;
        QMap<AbstractVisualizedData *, int> attributeLocations;
    };

    auto checkAttributeArrays = [] (AbstractVisualizedData * vis, vtkDataSetAttributes * attributes, int attributeLocation, QMap<QString, ArrayInfo> & arrayInfos) -> void
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
            ArrayInfo & arrayInfo = arrayInfos[name];

            int lastNumComp = arrayInfo.numComponents;
            int currentNumComp = dataArray->GetNumberOfComponents();

            if (lastNumComp && lastNumComp != currentNumComp)
            {
                qDebug() << "Array named" << name << "found with different number of components (" << lastNumComp << "," << dataArray->GetNumberOfComponents() << ")";
                continue;
            }
            if (arrayInfo.attributeLocations.contains(vis))
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
            checkAttributeArrays(vis, dataSet->GetPointData(), vtkAssignAttribute::POINT_DATA, arrayInfos);
            checkAttributeArrays(vis, dataSet->GetCellData(), vtkAssignAttribute::CELL_DATA, arrayInfos);
        }
    }

    QList<ColorMappingData *> instances;
    for (auto it = arrayInfos.begin(); it != arrayInfos.end(); ++it)
    {
        const auto & arrayInfo = it.value();
        AttributeArrayComponentMapping * mapping = new AttributeArrayComponentMapping(
            supportedData, 
            it.key(), 
            arrayInfo.numComponents,
            arrayInfo.attributeLocations);
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

AttributeArrayComponentMapping::AttributeArrayComponentMapping(
    const QList<AbstractVisualizedData *> & visualizedData, const QString & dataArrayName, 
    int numDataComponents, const QMap<AbstractVisualizedData *, int> & attributeLocations)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_dataArrayName(dataArrayName)
    , m_attributeLocations(attributeLocations)
{
    assert(!visualizedData.isEmpty());

    m_isValid = true;
}

QString AttributeArrayComponentMapping::name() const
{
    return m_dataArrayName;
}

QString AttributeArrayComponentMapping::scalarsName() const
{
    return m_dataArrayName;
}

vtkSmartPointer<vtkAlgorithm> AttributeArrayComponentMapping::createFilter(AbstractVisualizedData * visualizedData, int connection)
{
    int attributeLocation = m_attributeLocations.value(visualizedData, -1);

    if (attributeLocation == -1)
    {
        auto filter = vtkSmartPointer<vtkPassThrough>::New();
        filter->SetInputConnection(visualizedData->colorMappingInput(connection));
        return filter;
    }

    auto filter = vtkSmartPointer<vtkAssignAttribute>::New();
    filter->SetInputConnection(visualizedData->colorMappingInput(connection));
    filter->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS, attributeLocation);
    return filter;
}

bool AttributeArrayComponentMapping::usesFilter() const
{
    return true;
}

void AttributeArrayComponentMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * abstractMapper)
{
    ColorMappingData::configureMapper(visualizedData, abstractMapper);

    int attributeLocation = m_attributeLocations.value(visualizedData, -1);
    auto mapper = vtkMapper::SafeDownCast(abstractMapper);

    if (mapper && attributeLocation != -1)
    {
        mapper->ScalarVisibilityOn();

        if (attributeLocation == vtkAssignAttribute::CELL_DATA)
            mapper->SetScalarModeToUseCellData();
        else if (attributeLocation == vtkAssignAttribute::POINT_DATA)
            mapper->SetScalarModeToUsePointData();
        mapper->SelectColorArray(m_dataArrayName.toUtf8().data());
    }
    else if (mapper)
    {
        mapper->ScalarVisibilityOff();
    }
}

QMap<int, QPair<double, double>> AttributeArrayComponentMapping::updateBounds()
{
    QByteArray utf8Name = m_dataArrayName.toUtf8();

    QMap<int, QPair<double, double>> bounds;

    for (auto c = 0; c < numDataComponents(); ++c)
    {
        double totalMin = std::numeric_limits<double>::max();
        double totalMax = std::numeric_limits<double>::lowest();

        for (AbstractVisualizedData * visualizedData : m_visualizedData)
        {
            int attributeLocation = m_attributeLocations.value(visualizedData, -1);
            if (attributeLocation == -1)
                continue;

            for (int i = 0; i < visualizedData->numberOfColorMappingInputs(); ++i)
            {
                vtkDataSet * dataSet = visualizedData->colorMappingInputData(i);
                vtkDataArray * dataArray = 
                    attributeLocation == vtkAssignAttribute::CELL_DATA ? dataSet->GetCellData()->GetArray(utf8Name.data())
                    : (attributeLocation == vtkAssignAttribute::POINT_DATA ? dataArray = dataSet->GetPointData()->GetArray(utf8Name.data())
                    : nullptr);

                if (!dataArray)
                    continue;

                double range[2];
                dataArray->GetRange(range, c);

                // ignore arrays with invalid data
                if (range[1] < range[0])
                    continue;

                totalMin = std::min(totalMin, range[0]);
                totalMax = std::max(totalMax, range[1]);
            }
        }

        if (totalMin > totalMax)    // invalid data in all arrays on this component
        {
            totalMin = totalMax = 0;
        }

        bounds.insert(c, { totalMin, totalMax });
    }

    return bounds;
}
