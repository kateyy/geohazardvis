#include "RenderedVectorGrid3D.h"

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkLineSource.h>

#include <vtkPolyData.h>
#include <vtkDataSetAttributes.h>

#include <vtkGlyph3D.h>

#include <vtkPolyDataMapper.h>
#include <vtkPainterPolyDataMapper.h>
#include <vtkLinesPainter.h>

#include <vtkProperty.h>
#include <vtkLODActor.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


using namespace reflectionzeug;


RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject)
    : RenderedData(dataObject)
    , m_lineSource(vtkSmartPointer<vtkLineSource>::New())
    , m_glyph(vtkSmartPointer<vtkGlyph3D>::New())
{
    m_lineSource->SetPoint1(0.f, 0.f, 0.f);
    m_lineSource->SetPoint2(1.f, 0.f, 0.f);
    m_lineSource->SetResolution(1);
    m_lineSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

    m_glyph->SetSourceConnection(m_lineSource->GetOutputPort());
    m_glyph->ScalingOn();
    m_glyph->SetScaleModeToDataScalingOff();
    m_glyph->SetScaleFactor(0.1f);
    m_glyph->SetVectorModeToUseVector();

    m_glyph->SetInputDataObject(dataObject->dataSet());
}

RenderedVectorGrid3D::~RenderedVectorGrid3D() = default;

VectorGrid3DDataObject * RenderedVectorGrid3D::vectorGrid3DDataObject()
{
    return static_cast<VectorGrid3DDataObject *>(dataObject());
}

const VectorGrid3DDataObject * RenderedVectorGrid3D::vectorGrid3DDataObject() const
{
    return static_cast<const VectorGrid3DDataObject *>(dataObject());
}

PropertyGroup * RenderedVectorGrid3D::createConfigGroup()
{
    PropertyGroup * configGroup = new PropertyGroup();

    auto * renderSettings = new PropertyGroup("renderSettings");
    renderSettings->setOption("title", "rendering");
    configGroup->addProperty(renderSettings);

    auto * color = renderSettings->addProperty<Color>("color",
        [this]() {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });

    auto pointSize = renderSettings->addProperty<unsigned>("pointSize",
        [this]() {
        return static_cast<unsigned>(renderProperty()->GetPointSize());
    },
        [this](unsigned pointSize) {
        renderProperty()->SetPointSize(pointSize);
        emit geometryChanged();
    });
    pointSize->setOption("title", "point size");
    pointSize->setOption("minimum", 1);
    pointSize->setOption("maximum", 20);
    pointSize->setOption("step", 1);


    auto * vectorVisSettings = configGroup->addGroup("vectorVisSettings");
    vectorVisSettings->setOption("title", "vector visualization");
    
    auto prop_length = vectorVisSettings->addProperty<float>("length",
        [this]() { return static_cast<float>(m_glyph->GetScaleFactor());  },
        [this](float value) {
        m_glyph->SetScaleFactor(value);
        emit geometryChanged();
    });
    prop_length->setOption("step", 0.02f);

    return configGroup;
}

vtkProperty * RenderedVectorGrid3D::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    prop->SetColor(0, 0, 0);
    prop->SetInterpolationToFlat();
    prop->LightingOff();

    return prop;
}

vtkActor * RenderedVectorGrid3D::createActor()
{
    vtkLODActor * actor = vtkLODActor::New();
    VTK_CREATE(vtkPainterPolyDataMapper, mapper);
    VTK_CREATE(vtkLinesPainter, painter);
    mapper->SetPainter(painter);
    mapper->SetInputConnection(m_glyph->GetOutputPort());
    actor->SetMapper(mapper);

    return actor;
}
