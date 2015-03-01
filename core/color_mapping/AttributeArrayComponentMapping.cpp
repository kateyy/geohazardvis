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
#include <vtkPointData.h>
#include <vtkMapper.h>
#include <vtkAssignAttribute.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/vtkhelper.h>
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
        ArrayInfo(int comp = 0, int attr = -1)
            : numComponents(comp), attributeLocation(attr)
        {
        }
        int numComponents;
        int attributeLocation;
    };
    QMap<QString, ArrayInfo> arrayInfos;

    auto checkAddAttributeArrays = [&arrayInfos] (vtkDataSetAttributes * attributes, int attributeLocation) -> void
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

            QString name = QString::fromUtf8(dataArray->GetName());
            int lastNumComp = arrayInfos.value(name).numComponents;
            int currentNumComp = dataArray->GetNumberOfComponents();

            if (lastNumComp && lastNumComp != currentNumComp)
            {
                qDebug() << "found array named" << name << "with different number of components (" << lastNumComp << "," << dataArray->GetNumberOfComponents() << ")";
                continue;
            }

            if (!lastNumComp)   // current array isn't yet in our list
                arrayInfos.insert(name, { currentNumComp, attributeLocation });
        }
    };

    QList<AbstractVisualizedData *> supportedData;

    // list all available array names, check for same number of components
    for (AbstractVisualizedData * vis : visualizedData)
    {
        if (vis->contentType() == ContentType::Context2D)   // don't try to map colors to a plot
            continue;

        supportedData << vis;

        for (int i = 0; i < vis->numberOfColorMappingInputs(); ++i)
        {
            vtkDataSet * dataSet = vis->colorMappingInputData(i);

            checkAddAttributeArrays(dataSet->GetCellData(), vtkAssignAttribute::CELL_DATA);
            checkAddAttributeArrays(dataSet->GetPointData(), vtkAssignAttribute::POINT_DATA);
        }
    }

    QList<ColorMappingData *> instances;
    for (auto it = arrayInfos.begin(); it != arrayInfos.end(); ++it)
    {
        const auto & arrayInfo = it.value();
        AttributeArrayComponentMapping * mapping = new AttributeArrayComponentMapping(supportedData, it.key(), arrayInfo.attributeLocation, arrayInfo.numComponents);
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

AttributeArrayComponentMapping::AttributeArrayComponentMapping(const QList<AbstractVisualizedData *> & visualizedData, QString dataArrayName, int attributeLocation, vtkIdType numDataComponents)
    : ColorMappingData(visualizedData, numDataComponents)
    , m_attributeLocation(attributeLocation)
    , m_dataArrayName(dataArrayName)
{
    assert(!visualizedData.isEmpty());

    m_isValid = true;
}

AttributeArrayComponentMapping::~AttributeArrayComponentMapping() = default;

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
    VTK_CREATE(vtkAssignAttribute, filter);

    filter->SetInputConnection(visualizedData->colorMappingInput(connection));
    filter->Assign(m_dataArrayName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
        m_attributeLocation);

    return filter;
}

bool AttributeArrayComponentMapping::usesFilter() const
{
    return true;
}

void AttributeArrayComponentMapping::configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper)
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
    }
}

QMap<vtkIdType, QPair<double, double>> AttributeArrayComponentMapping::updateBounds()
{
    QByteArray utf8Name = m_dataArrayName.toUtf8();

    QMap<vtkIdType, QPair<double, double>> bounds;

    for (vtkIdType c = 0; c < numDataComponents(); ++c)
    {
        double totalMin = std::numeric_limits<double>::max();
        double totalMax = std::numeric_limits<double>::lowest();

        for (AbstractVisualizedData * visualizedData : m_visualizedData)
        {
            for (int i = 0; i < visualizedData->numberOfColorMappingInputs(); ++i)
            {
                vtkDataSet * dataSet = visualizedData->colorMappingInputData(i);
                vtkDataArray * dataArray = nullptr;
                if (m_attributeLocation == vtkAssignAttribute::CELL_DATA)
                    dataArray = dataSet->GetCellData()->GetArray(utf8Name.data());
                else if (m_attributeLocation == vtkAssignAttribute::POINT_DATA)
                    dataArray = dataSet->GetPointData()->GetArray(utf8Name.data());

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
