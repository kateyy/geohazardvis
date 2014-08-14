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

    m_arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    m_arrowSource->SetShaftRadius(0.02);
    m_arrowSource->SetTipRadius(0.07);
    m_arrowSource->SetTipLength(0.3);

    m_arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    m_arrowGlyph->SetScaleModeToScaleByScalar();
    m_arrowGlyph->SetVectorModeToUseNormal();
    m_arrowGlyph->OrientOn();
    m_arrowGlyph->SetSourceConnection(m_arrowSource->GetOutputPort());
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

float NormalRepresentation::arrowLength() const
{
    return static_cast<float>(m_arrowGlyph->GetScaleFactor());
}

void NormalRepresentation::setArrowLength(float length)
{
    m_arrowGlyph->SetScaleFactor(length);

    emit geometryChanged();
}

float NormalRepresentation::arrowRadius() const
{
    return (float)m_arrowSource->GetTipRadius();
}

void NormalRepresentation::setArrowRadius(float radius)
{
    m_arrowSource->SetTipRadius(radius);
    m_arrowSource->SetShaftRadius(radius * 0.1f);

    emit geometryChanged();
}

float NormalRepresentation::arrowTipLength() const
{
    return (float)m_arrowSource->GetTipLength();
}

void NormalRepresentation::setArrowTipLength(float tipLength)
{
    m_arrowSource->SetTipLength(tipLength);

    emit geometryChanged();
}

PropertyGroup * NormalRepresentation::createPropertyGroup()
{
    PropertyGroup * group = new PropertyGroup("normals");
    auto * prop_visible = group->addProperty<bool>("visible",
        std::bind(&NormalRepresentation::visible, this),
        std::bind(&NormalRepresentation::setVisible, this, std::placeholders::_1));

    auto prop_length = group->addProperty<float>("length", this,
        &NormalRepresentation::arrowLength, &NormalRepresentation::setArrowLength);
    prop_length->setTitle("arrow length");
    prop_length->setMinimum(0.00001f);
    prop_length->setStep(0.1f);

    auto prop_radius = group->addProperty<float>("radius", this,
        &NormalRepresentation::arrowRadius, &NormalRepresentation::setArrowRadius);
    prop_radius->setTitle("tip radius");
    prop_radius->setMinimum(0.00001f);
    prop_radius->setStep(0.01f);

    auto prop_tipLength = group->addProperty<float>("tipLength", this,
        &NormalRepresentation::arrowTipLength, &NormalRepresentation::setArrowTipLength);
    prop_tipLength->setTitle("tip length");
    prop_tipLength->setMinimum(0.00001f);
    prop_tipLength->setMaximum(1.f);
    prop_tipLength->setStep(0.01f);

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
        m_mapper = vtkSmartPointer<vtkDataSetMapper>::New();
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