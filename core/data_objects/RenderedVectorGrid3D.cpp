#include "RenderedVectorGrid3D.h"

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkLineSource.h>
#include <vtkAppendPolyData.h>

#include <vtkPolyData.h>
#include <vtkDataSetAttributes.h>

#include <vtkGlyph3D.h>
#include <vtkMaskPoints.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkLODActor.h>

#include "config.h"
#if VTK_RENDERING_BACKEND == 1
#include <vtkPainterPolyDataMapper.h>
#include <vtkLinesPainter.h>
#endif


#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


using namespace reflectionzeug;


namespace
{

vtkSmartPointer<vtkAlgorithm> createArrow()
{
    VTK_CREATE(vtkLineSource, shaft);
    shaft->SetPoint1(0.f, 0.f, 0.f);
    shaft->SetPoint2(1.f, 0.f, 0.f);

    VTK_CREATE(vtkLineSource, cone1);
    cone1->SetPoint1(1.00f, 0.0f, 0.f);
    cone1->SetPoint2(0.65f, 0.1f, 0.f);

    VTK_CREATE(vtkLineSource, cone2);
    cone2->SetPoint1(1.00f,  0.0f, 0.f);
    cone2->SetPoint2(0.65f, -0.1f, 0.f);

    VTK_CREATE(vtkAppendPolyData, arrow);
    arrow->AddInputConnection(shaft->GetOutputPort());
    arrow->AddInputConnection(cone2->GetOutputPort());
    arrow->AddInputConnection(cone2->GetOutputPort());

    return arrow;
}

}

RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject)
    : RenderedData(dataObject)
    , m_glyph(vtkSmartPointer<vtkGlyph3D>::New())
    , m_lodMask(vtkSmartPointer<vtkMaskPoints>::New())
{
    vtkSmartPointer<vtkAlgorithm> arrow = createArrow();
    
    m_glyph->SetSourceConnection(arrow->GetOutputPort());
    m_glyph->ScalingOn();
    m_glyph->SetScaleModeToDataScalingOff();
    m_glyph->SetScaleFactor(0.1f);
    m_glyph->SetVectorModeToUseVector();

    m_lodMask->SetInputDataObject(dataObject->dataSet());
    m_lodMask->RandomModeOn();
    m_lodMask->SetRandomModeType(2);
    m_lodMask->SetOnRatio(1);
    m_lodMask->SetMaximumNumberOfPoints(10000);

    m_glyph->SetInputConnection(m_lodMask->GetOutputPort());
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

    auto * renderSettings = configGroup->addGroup("renderSettings");
    renderSettings->setOption("title", "rendering");

    auto * color = renderSettings->addProperty<Color>("lineColor",
        [this]() {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    color->setOption("title", "line color");

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
    pointSize->setOption("maximum", 100);
    pointSize->setOption("step", 1);

    auto lineWidth = renderSettings->addProperty<unsigned>("lineWidth",
        [this]() {
        return static_cast<unsigned>(renderProperty()->GetLineWidth());
    },
        [this](unsigned lineWidth) {
        renderProperty()->SetLineWidth(lineWidth);
        emit geometryChanged();
    });
    lineWidth->setOption("title", "line width");
    lineWidth->setOption("minimum", 1);
    lineWidth->setOption("maximum", 100);
    lineWidth->setOption("step", 1);
    
    auto lineLength = renderSettings->addProperty<float>("lineLength",
        [this]() { return static_cast<float>(m_glyph->GetScaleFactor());  },
        [this](float value) {
        m_glyph->SetScaleFactor(value);
        emit geometryChanged();
    });
    lineLength->setOption("title", "line length");
    lineLength->setOption("minimum", 0);
    lineLength->setOption("step", 0.02f);


    auto * lodSettings = configGroup->addGroup("lod");
    lodSettings->setOption("title", "level of detail");

    auto lodOuputPoints = lodSettings->addProperty<vtkIdType>("lodOuputPoints",
        [this]() { return m_lodMask->GetMaximumNumberOfPoints(); },
        [this](vtkIdType value) {
        m_lodMask->SetMaximumNumberOfPoints(value);
        emit geometryChanged();
    });
    lodOuputPoints->setOption("title", "maximum number of points");
    lodOuputPoints->setOption("minimum", m_lodMask->GetMaximumNumberOfPointsMinValue());
    lodOuputPoints->setOption("maximum", m_lodMask->GetMaximumNumberOfPointsMaxValue());

    return configGroup;
}

vtkProperty * RenderedVectorGrid3D::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    prop->SetColor(1, 0, 0);
    prop->SetInterpolationToFlat();
    prop->LightingOff();

    return prop;
}

vtkActor * RenderedVectorGrid3D::createActor()
{
    vtkLODActor * actor = vtkLODActor::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper;
#if VTK_RENDERING_BACKEND == 2
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
#else
    VTK_CREATE(vtkLinesPainter, painter);
    VTK_CREATE(vtkPainterPolyDataMapper, painterMapper);
    painterMapper->SetPainter(painter);
    mapper = painterMapper;
#endif 
    mapper->SetInputConnection(m_glyph->GetOutputPort());
    actor->SetMapper(mapper);

    return actor;
}
