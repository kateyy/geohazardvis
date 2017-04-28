#include "PointCloudDataAttributeGlyphMapping.h"

#include <algorithm>

#include <QDebug>

#include <vtkAssignAttribute.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>
#include <vtkPointData.h>
#include <vtkGlyph3D.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedPointCloudData.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


namespace
{
const QString s_name = "point cloud data attribute vectors";
}

const bool PointCloudDataAttributeGlyphMapping::s_registered =
    GlyphMappingRegistry::instance().registerImplementation(
        s_name,
        newInstances);


std::vector<std::unique_ptr<GlyphMappingData>> PointCloudDataAttributeGlyphMapping::newInstances(
    RenderedData & renderedData)
{
    auto renderedPointCloud = dynamic_cast<RenderedPointCloudData *>(&renderedData);
    if (!renderedPointCloud)
    {
        return{};
    }

    auto processedData = renderedPointCloud->processedOutputDataSet();
    if (!processedData)
    {
        return{};
    }

    const vtkIdType numPoints = processedData->GetNumberOfPoints();
    auto & pointData = *processedData->GetPointData();
    
    std::vector<QString> arrayNames;
    for (int i = 0; i < pointData.GetNumberOfArrays(); ++i)
    {
        auto a = pointData.GetArray(i);

        if (!a || a->GetNumberOfComponents() != 3 || a->GetNumberOfTuples() != numPoints)
        {
            continue;
        }

        if (a->GetInformation()->Has(DataObject::ARRAY_IS_AUXILIARY())
            && a->GetInformation()->Get(DataObject::ARRAY_IS_AUXILIARY()))
        {
            continue;
        }


        const auto name = QString::fromUtf8(a->GetName());

        if (std::find(arrayNames.begin(), arrayNames.end(), name) != arrayNames.end())
        {
            qWarning() << "Duplicated point array name """ + name + """ in data object """
                + renderedData.dataObject().name() + """";
            continue;
        }

        arrayNames.push_back(name);
    }

    std::vector<std::unique_ptr<GlyphMappingData>> instances;
    for (auto && vectorName : arrayNames)
    {
        auto mapping = std::make_unique<PointCloudDataAttributeGlyphMapping>(
            *renderedPointCloud, vectorName);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

PointCloudDataAttributeGlyphMapping::PointCloudDataAttributeGlyphMapping(
    RenderedData & renderedData,
    const QString & attributeName)
    : GlyphMappingData(renderedData)
    , m_attributeName{ attributeName }
{
    arrowGlyph()->SetVectorModeToUseVector();

    m_assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignVectors->SetInputConnection(renderedData.processedOutputPort());
    m_assignVectors->Assign(m_attributeName.toUtf8().data(),
        vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
}

QString PointCloudDataAttributeGlyphMapping::name() const
{
    return m_attributeName;
}

IndexType PointCloudDataAttributeGlyphMapping::scalarsAssociation() const
{
    return IndexType::points;
}

vtkAlgorithmOutput * PointCloudDataAttributeGlyphMapping::vectorDataOutputPort()
{
    return m_assignVectors->GetOutputPort();
}
