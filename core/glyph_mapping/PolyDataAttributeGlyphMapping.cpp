/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PolyDataAttributeGlyphMapping.h"

#include <QDebug>

#include <vtkAssignAttribute.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>
#include <vtkPointData.h>
#include <vtkGlyph3D.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


namespace
{
const QString s_name = "poly data attribute vectors";
}

const bool PolyDataAttributeGlyphMapping::s_registered = GlyphMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

std::vector<std::unique_ptr<GlyphMappingData>> PolyDataAttributeGlyphMapping::newInstances(RenderedData & renderedData)
{
    auto renderedPolyData = dynamic_cast<RenderedPolyData *>(&renderedData);

    // only polygonal datasets are supported
    if (!renderedPolyData)
    {
        return{};
    }

    // find 3D-vector data, skip the "centroid" attribute array

    auto centroids = renderedPolyData->transformedCellCenterDataSet();
    if (!centroids)
    {
        return{};
    }

    auto pointData = centroids->GetPointData();
    const vtkIdType numPoints = centroids->GetNumberOfPoints();

    QStringList vectorArrayNames;
    for (int i = 0; i < pointData->GetNumberOfArrays(); ++i)
    {
        auto a = pointData->GetArray(i);

        if (!a || a->GetNumberOfComponents() != 3 || a->GetNumberOfTuples() != numPoints)
        {
            continue;
        }

        const auto name = QString::fromUtf8(a->GetName());

        if (name.toLower() == "centroid")
        {
            continue;
        }

        if (a->GetInformation()->Has(DataObject::ARRAY_IS_AUXILIARY())
            && a->GetInformation()->Get(DataObject::ARRAY_IS_AUXILIARY()))
        {
            continue;
        }

        if (vectorArrayNames.contains(name))
        {
            qWarning() << "Duplicated cell array name """ + name + """ in data object """ + renderedPolyData->dataObject().name() + """";
            continue;
        }

        vectorArrayNames << name;
    }

    std::vector<std::unique_ptr<GlyphMappingData>> instances;
    for (auto && vectorName : vectorArrayNames)
    {
        auto mapping = std::make_unique<PolyDataAttributeGlyphMapping>(*renderedPolyData, vectorName);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

PolyDataAttributeGlyphMapping::PolyDataAttributeGlyphMapping(
    RenderedPolyData & renderedData,
    const QString & attributeName)
    : GlyphMappingData(renderedData)
    , m_attributeName{ attributeName }
{
    arrowGlyph()->SetVectorModeToUseVector();

    m_assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignVectors->SetInputConnection(renderedData.transformedCellCenterOutputPort());
    m_assignVectors->Assign(m_attributeName.toUtf8().data(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
}

QString PolyDataAttributeGlyphMapping::name() const
{
    return m_attributeName;
}

IndexType PolyDataAttributeGlyphMapping::scalarsAssociation() const
{
    return IndexType::cells;
}

vtkAlgorithmOutput * PolyDataAttributeGlyphMapping::vectorDataOutputPort()
{
    return m_assignVectors->GetOutputPort();
}
