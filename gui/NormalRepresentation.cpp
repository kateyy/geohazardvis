#include "NormalRepresentation.h"

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
, m_polyDataChanged(false)
, m_actor(vtkSmartPointer<vtkActor>::New())
{
    m_actor->SetVisibility(m_visible);
    m_actor->PickableOff();

    VTK_CREATE(vtkArrowSource, arrow);
    arrow->SetShaftRadius(0.02);
    arrow->SetTipRadius(0.07);
    arrow->SetTipLength(0.3);

    m_arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    m_arrowGlyph->SetScaleModeToScaleByScalar();
    m_arrowGlyph->SetVectorModeToUseNormal();
    m_arrowGlyph->OrientOn();
    m_arrowGlyph->SetSourceConnection(arrow->GetOutputPort());
}

void NormalRepresentation::setData(vtkPolyData * polyData)
{
    if (m_polyData == polyData)
        return;

    m_polyData = polyData;
    m_polyDataChanged = true;

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

void NormalRepresentation::setGlyphSize(float size)
{
    m_arrowGlyph->SetScaleFactor(size);

    emit geometryChanged();
}

float NormalRepresentation::glyphSize() const
{
    return static_cast<float>(m_arrowGlyph->GetScaleFactor());
}

PropertyGroup * NormalRepresentation::createPropertyGroup()
{
    PropertyGroup * group = new PropertyGroup("normals");
    auto * prop_visible = group->addProperty<bool>("visible",
        std::bind(&NormalRepresentation::visible, this),
        std::bind(&NormalRepresentation::setVisible, this, std::placeholders::_1));

    auto prop_size = group->addProperty<float>("size",
        std::bind(&NormalRepresentation::glyphSize, this),
        std::bind(&NormalRepresentation::setGlyphSize, this, std::placeholders::_1));
    prop_size->setTitle("arrow length");
    prop_size->setMinimum(0.00001f);
    prop_size->setStep(0.1);

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
    if (m_polyDataChanged && !m_polyData->GetPointData()->HasArray("Normals"))
    {
        VTK_CREATE(vtkPolyDataNormals, inputNormals);
        inputNormals->ComputeCellNormalsOff();
        inputNormals->ComputePointNormalsOn();
        inputNormals->SetInputDataObject(m_polyData);
        inputNormals->Update();

        m_polyData->GetPointData()->SetNormals(inputNormals->GetOutput()->GetPointData()->GetNormals());
    }

    // create arrow geometry if needed
    if (!m_mapper)
    {
        m_mapper = vtkDataSetMapper::New();
        m_mapper->SetInputConnection(m_arrowGlyph->GetOutputPort());
    }

    if (m_polyDataChanged) {
        double * bounds = m_polyData->GetBounds();
        double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));
        m_arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);
        m_arrowGlyph->SetInputData(m_polyData);
    }

    m_actor->SetMapper(m_mapper);
    m_actor->SetVisibility(m_visible);

    emit geometryChanged();

    m_polyDataChanged = false;
}

vtkActor * NormalRepresentation::actor()
{
    return m_actor;
}