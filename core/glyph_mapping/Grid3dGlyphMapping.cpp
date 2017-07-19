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

#include "Grid3dGlyphMapping.h"

#include <vtkActor.h>
#include <vtkAssignAttribute.h>
#include <vtkGlyph3D.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>
#include <vtkPointData.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedVectorGrid3D.h>
#include <core/glyph_mapping/GlyphMappingRegistry.h>


namespace
{
const QString s_name = "grid 3d vectors";
}

const bool Grid3dGlyphMapping::s_registered = GlyphMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

using namespace reflectionzeug;


std::vector<std::unique_ptr<GlyphMappingData>> Grid3dGlyphMapping::newInstances(RenderedData & renderedData)
{
    auto renderedGrid = dynamic_cast<RenderedVectorGrid3D *>(&renderedData);
    if (!renderedGrid)
    {
        return{};
    }

    auto resampledChecked = renderedGrid->resampledDataSet();
    if (!resampledChecked)
    {
        return{};
    }
    auto pointData = resampledChecked->GetPointData();
    std::vector<vtkDataArray *> vectorArrays;
    for (int i = 0; auto a = pointData->GetArray(i); ++i)
    {
        assert(a);

        if (a->GetInformation()->Has(DataObject::ARRAY_IS_AUXILIARY())
            && a->GetInformation()->Get(DataObject::ARRAY_IS_AUXILIARY()))
        {
            continue;
        }

        if (a->GetNumberOfComponents() != 3)
        {
            continue;
        }

        vectorArrays.emplace_back(a);
    }

    std::vector<std::unique_ptr<GlyphMappingData>> instances;
    for (auto vectorArray : vectorArrays)
    {
        auto mapping = std::make_unique<Grid3dGlyphMapping>(*renderedGrid, vectorArray);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances.push_back(std::move(mapping));
        }
    }

    return instances;
}

Grid3dGlyphMapping::Grid3dGlyphMapping(RenderedVectorGrid3D & renderedGrid, vtkDataArray * dataArray)
    : GlyphMappingData(renderedGrid)
    , m_renderedGrid{ renderedGrid }
    , m_dataArray{ dataArray }
{
    setVisible(true);

    actor()->PickableOn();
    arrowGlyph()->SetVectorModeToUseVector();
    setRepresentation(Representation::SimpleArrow);
    setColor(1, 0, 0);
    updateArrowLength();

    connect(&renderedGrid, &RenderedVectorGrid3D::sampleRateChanged, [this] (int, int, int) {
        this->updateArrowLength();
    });

    m_assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    m_assignVectors->SetInputConnection(m_renderedGrid.resampledOuputPort());
    m_assignVectors->Assign(m_dataArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
}

QString Grid3dGlyphMapping::name() const
{
    assert(m_dataArray);
    return QString::fromUtf8(m_dataArray->GetName());
}

IndexType Grid3dGlyphMapping::scalarsAssociation() const
{
    return IndexType::points;
}

vtkAlgorithmOutput * Grid3dGlyphMapping::vectorDataOutputPort()
{
    return m_assignVectors->GetOutputPort();
}

void Grid3dGlyphMapping::updateArrowLength()
{
    auto resampledChecked = m_renderedGrid.resampledDataSet();
    if (!resampledChecked)
    {
        return;
    }
    double cellSpacing = resampledChecked->GetSpacing()[0];
    arrowGlyph()->SetScaleFactor(0.75 * cellSpacing);

    emit geometryChanged();
}
