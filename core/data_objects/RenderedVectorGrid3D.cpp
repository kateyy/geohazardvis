#include "RenderedVectorGrid3D.h"

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkPolyData.h>

#include <vtkPolyDataMapper.h>
#include <vtkPainterPolyDataMapper.h>
#include <vtkPointsPainter.h>

#include <vtkProperty.h>
#include <vtkLODActor.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


using namespace reflectionzeug;


RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject)
    : RenderedData(dataObject)
{
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
    VTK_CREATE(vtkPainterPolyDataMapper, mapper);

    mapper->GetInformation()->Set(DataObject::NameKey(),
        dataObject()->name().toLatin1().data());

    vtkSmartPointer<vtkPointsPainter> painter = vtkSmartPointer<vtkPointsPainter>::New();
    mapper->ScalarVisibilityOff();
    mapper->SetPainter(painter);

    mapper->SetInputDataObject(dataObject()->dataSet());

    vtkLODActor * actor = vtkLODActor::New();
    actor->SetMapper(mapper);

    return actor;
}
