#include "normalrepresentation.h"

#include <algorithm>

#include <vtkPolyData.h>
#include <vtkPointData.h>

#include <vtkArrowSource.h>

#include <vtkGlyph3D.h>
#include <vtkPolyDataNormals.h>

#include <vtkActor.h>

#include <vtkDataSetMapper.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include "core/vtkhelper.h"

using namespace reflectionzeug;

NormalRepresentation::NormalRepresentation()
: m_visible(true)
, m_actor(vtkSmartPointer<vtkActor>::New())
{
    m_actor->SetVisibility(m_visible);
    m_actor->PickableOff();
}

void NormalRepresentation::setData(vtkPolyData * polyData)
{
    if (m_polyData == polyData)
        return;

    m_polyData = polyData;

    updateGlyphs();
}

void NormalRepresentation::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;

    updateGlyphs();
}

bool NormalRepresentation::visible() const
{
    return m_visible;
}

PropertyGroup * NormalRepresentation::createPropertyGroup()
{
    PropertyGroup * group = new PropertyGroup("vertexNormals");
    auto * prop_visible = group->addProperty<bool>("visible",
        std::bind(&NormalRepresentation::visible, this),
        std::bind(&NormalRepresentation::setVisible, this, std::placeholders::_1));

    return group;
}

void NormalRepresentation::updateGlyphs()
{
    if (!m_polyData) {
        m_mapper = nullptr;
        m_actor->SetMapper(nullptr);
        return;
    }

    // compute vertex normals if needed
    if (!m_polyData->GetPointData()->HasArray("Normals"))
    {
        VTK_CREATE(vtkPolyDataNormals, inputNormals);
        inputNormals->ComputeCellNormalsOff();
        inputNormals->ComputePointNormalsOn();
        inputNormals->SetInputDataObject(m_polyData);
        inputNormals->Update();

        m_polyData->GetPointData()->SetNormals(inputNormals->GetOutput()->GetPointData()->GetNormals());
    }

    VTK_CREATE(vtkArrowSource, arrow);
    arrow->SetShaftRadius(0.02);
    arrow->SetTipRadius(0.07);
    arrow->SetTipLength(0.3);

    VTK_CREATE(vtkGlyph3D, arrowGlyph);

    double * bounds = m_polyData->GetBounds();
    double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));

    arrowGlyph->SetScaleModeToScaleByScalar();
    arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);
    arrowGlyph->SetVectorModeToUseNormal();
    arrowGlyph->OrientOn();
    arrowGlyph->SetInputData(m_polyData);
    arrowGlyph->SetSourceConnection(arrow->GetOutputPort());

    m_mapper = vtkDataSetMapper::New();
    m_mapper->SetInputConnection(arrowGlyph->GetOutputPort());

    m_actor->SetMapper(m_mapper);
    m_actor->SetVisibility(m_visible);

    emit geometryChanged();
}

vtkActor * NormalRepresentation::actor()
{
    return m_actor;
}