#include "SurfaceNormalMapping.h"

#include <algorithm>

#include <vtkPolyData.h>

#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPoints.h>

#include <vtkArrowSource.h>

#include <vtkGlyph3D.h>
#include <vtkPolyDataNormals.h>

#include <vtkVertexGlyphFilter.h>

#include <vtkActor.h>
#include <vtkProperty.h>

#include <vtkDataSetMapper.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include "core/vtkhelper.h"


using namespace reflectionzeug;

SurfaceNormalMapping::SurfaceNormalMapping()
: m_visible(true)
, m_showDispVecs(true)
, m_normalType(NormalType::CellNormal)
, m_polyDataChanged(false)
, m_normalTypeChanged(false)
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

void SurfaceNormalMapping::setData(vtkPolyData * polyData)
{
    if (m_polyData == polyData)
        return;

    m_polyData = polyData;
    m_polyDataChanged = true;

    updateGlyphs();
}

void SurfaceNormalMapping::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;

    updateGlyphs();
}

bool SurfaceNormalMapping::visible() const
{
    return m_visible;
}

float SurfaceNormalMapping::arrowLength() const
{
    return static_cast<float>(m_arrowGlyph->GetScaleFactor());
}

void SurfaceNormalMapping::setArrowLength(float length)
{
    m_arrowGlyph->SetScaleFactor(length);

    emit geometryChanged();
}

float SurfaceNormalMapping::arrowRadius() const
{
    return (float)m_arrowSource->GetTipRadius();
}

void SurfaceNormalMapping::setArrowRadius(float radius)
{
    m_arrowSource->SetTipRadius(radius);
    m_arrowSource->SetShaftRadius(radius * 0.1f);

    emit geometryChanged();
}

float SurfaceNormalMapping::arrowTipLength() const
{
    return (float)m_arrowSource->GetTipLength();
}

void SurfaceNormalMapping::setArrowTipLength(float tipLength)
{
    m_arrowSource->SetTipLength(tipLength);

    emit geometryChanged();
}

PropertyGroup * SurfaceNormalMapping::createPropertyGroup()
{
    PropertyGroup * group = new PropertyGroup("normals");
    auto * prop_visible = group->addProperty<bool>("visible",
        std::bind(&SurfaceNormalMapping::visible, this),
        std::bind(&SurfaceNormalMapping::setVisible, this, std::placeholders::_1));

    auto * prop_showDispVecs = group->addProperty<bool>("showDispVecs",
        [this](){ return m_showDispVecs;
    },
        [this](bool d) {
        m_showDispVecs = d;
        m_normalTypeChanged = true;
        updateGlyphs();
    });
    prop_showDispVecs->setTitle("displacement vectors");

    auto * prop_normalType = group->addProperty<NormalType>("type",
        [this](){ return m_normalType;
    },
        [this](NormalType type) { 
        m_normalType = type;
        m_normalTypeChanged = true;
        updateGlyphs();
    });
    prop_normalType->setStrings({
            { NormalType::CellNormal, "cell normal" },
            { NormalType::PointNormal, "point normal" }
    });

    auto prop_length = group->addProperty<float>("length", this,
        &SurfaceNormalMapping::arrowLength, &SurfaceNormalMapping::setArrowLength);
    prop_length->setTitle("arrow length");
    prop_length->setMinimum(0.00001f);
    prop_length->setStep(0.1f);

    auto prop_radius = group->addProperty<float>("radius", this,
        &SurfaceNormalMapping::arrowRadius, &SurfaceNormalMapping::setArrowRadius);
    prop_radius->setTitle("tip radius");
    prop_radius->setMinimum(0.00001f);
    prop_radius->setStep(0.01f);

    auto prop_tipLength = group->addProperty<float>("tipLength", this,
        &SurfaceNormalMapping::arrowTipLength, &SurfaceNormalMapping::setArrowTipLength);
    prop_tipLength->setTitle("tip length");
    prop_tipLength->setMinimum(0.00001f);
    prop_tipLength->setMaximum(1.f);
    prop_tipLength->setStep(0.01f);

    auto * edgeColor = group->addProperty<Color>("color",
        [this]() {
        double * color = m_actor->GetProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        m_actor->GetProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    edgeColor->setTitle("color");

    return group;
}

void SurfaceNormalMapping::updateGlyphs()
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
        inputNormals->ComputeCellNormalsOn();
        inputNormals->ComputePointNormalsOn();
        inputNormals->SetInputDataObject(m_polyData);
        inputNormals->Update();

        m_polyData->GetPointData()->SetNormals(inputNormals->GetOutput()->GetPointData()->GetNormals());
        m_polyData->GetCellData()->SetNormals(inputNormals->GetOutput()->GetCellData()->GetNormals());
    }

    // create arrow geometry if needed
    if (!m_mapper)
    {
        m_mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        m_mapper->SetInputConnection(m_arrowGlyph->GetOutputPort());
    }

    if (m_polyDataChanged)
    {
        double * bounds = m_polyData->GetBounds();
        double maxBoundsSize = std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[5] - bounds[4]));
        m_arrowGlyph->SetScaleFactor(maxBoundsSize * 0.1);
    }

    if (m_polyDataChanged || m_normalTypeChanged)
    {
        vtkDataArray * centroid = m_polyData->GetCellData()->GetArray("centroid");
        vtkDataArray * dispVecs = m_polyData->GetCellData()->GetArray("dispVec");

        if (dispVecs && m_showDispVecs)
            m_arrowGlyph->SetVectorModeToUseVector();
        else
            m_arrowGlyph->SetVectorModeToUseNormal();

        if (m_normalType == NormalType::CellNormal && centroid)
        {
            VTK_CREATE(vtkPoints, points);
            points->SetData(centroid);

            VTK_CREATE(vtkPolyData, pointsPolyData);
            pointsPolyData->SetPoints(points);

            VTK_CREATE(vtkVertexGlyphFilter, filter);
            filter->SetInputData(pointsPolyData);
            filter->Update();
            vtkPolyData * processedPoints = filter->GetOutput();

            processedPoints->GetPointData()->SetNormals(m_polyData->GetCellData()->GetNormals());
            processedPoints->GetPointData()->SetVectors(centroid);

            m_arrowGlyph->SetInputData(processedPoints);
        }
        else
        {
            m_arrowGlyph->SetInputData(m_polyData);
        }

        m_normalTypeChanged = false;
    }

    m_actor->SetMapper(m_mapper);
    m_actor->SetVisibility(m_visible);

    emit geometryChanged();

    m_polyDataChanged = false;
}

vtkActor * SurfaceNormalMapping::actor()
{
    return m_actor;
}