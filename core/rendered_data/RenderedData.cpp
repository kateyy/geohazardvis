#include "RenderedData.h"
#include "RenderedData_private.h"

#include <cassert>

#include <vtkExecutive.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/macros.h>


using namespace reflectionzeug;


RenderedData::RenderedData(ContentType contentType, CoordinateTransformableDataObject & dataObject)
    : AbstractVisualizedData(std::make_unique<RenderedData_private>(*this, contentType, dataObject))
{
    const auto darkGray = 0.2;
    dPtr().outlineProperty->SetColor(darkGray, darkGray, darkGray);

    dPtr().transformedCoordinatesOutput->SetInputConnection(dataObject.processedOutputPort());
}

RenderedData::~RenderedData() = default;

CoordinateTransformableDataObject & RenderedData::transformableObject()
{
    return static_cast<CoordinateTransformableDataObject &>(dataObject());
}

const CoordinateTransformableDataObject & RenderedData::transformableObject() const
{
    return static_cast<const CoordinateTransformableDataObject &>(dataObject());
}

void RenderedData::setDefaultCoordinateSystem(const CoordinateSystemSpecification & coordinateSystem)
{
    if (transformableObject().canTransformTo(coordinateSystem))
    {
        dPtr().transformedCoordinatesOutput->SetInputConnection(
            transformableObject().coordinateTransformedOutputPort(coordinateSystem));
        dPtr().defaultCoordinateSystem = coordinateSystem;
    }
    else
    {
        dPtr().transformedCoordinatesOutput->SetInputConnection(
            dataObject().processedOutputPort());
        dPtr().defaultCoordinateSystem = CoordinateSystemSpecification();
    }

    invalidateVisibleBounds();
}

const CoordinateSystemSpecification & RenderedData::defaultCoordinateSystem() const
{
    return dPtr().defaultCoordinateSystem;
}

vtkAlgorithmOutput * RenderedData::transformedCoordinatesOutputPort()
{
    return dPtr().transformedCoordinatesOutput->GetOutputPort();
}

vtkDataSet * RenderedData::transformedCoordinatesDataSet()
{
    if (dPtr().transformedCoordinatesOutput->GetExecutive()->Update() == 0)
    {
        return nullptr;
    }
    return vtkDataSet::SafeDownCast(dPtr().transformedCoordinatesOutput->GetOutput());
}

RenderedData::Representation RenderedData::representation() const
{
    return dPtr().representation;
}

void RenderedData::setRepresentation(Representation representation)
{
    if (dPtr().representation == representation)
    {
        return;
    }

    dPtr().representation = representation;

    representationChangedEvent(representation);

    invalidateViewProps();
}

std::unique_ptr<PropertyGroup> RenderedData::createConfigGroup()
{
    auto group = std::make_unique<PropertyGroup>();

    group->addProperty<Representation>("Representation", this, 
        &RenderedData::representation, &RenderedData::setRepresentation)
        ->setStrings({
            { Representation::content, "Content" },
            { Representation::outline, "Outline" },
            { Representation::both, "Content and Outline" }
    });

    group->addProperty<unsigned>("outlineWidth",
        [this] () {
        return static_cast<unsigned>(dPtr().outlineProperty->GetLineWidth());
    },
        [this] (unsigned width) {
        dPtr().outlineProperty->SetLineWidth(static_cast<float>(width));
        emit geometryChanged();
    })
        ->setOptions({
            { "title", "Outline Width" },
            { "minimum", 1 },
            { "maximum", std::numeric_limits<unsigned>::max() },
            { "step", 1 },
            { "suffix", " pixel" }
    });

    group->addProperty<Color>("outlineColor",
        [this] () {
        double * color = dPtr().outlineProperty->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this] (const Color & color) {
        dPtr().outlineProperty->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    })
        ->setOption("title", "Outline Color");

    return group;
}

vtkSmartPointer<vtkPropCollection> RenderedData::viewProps()
{
    if (!dPtr().viewPropsInvalid)
    {
        assert(dPtr().viewProps);
        return dPtr().viewProps;
    }

    dPtr().viewPropsInvalid = false;

    if (!dPtr().viewProps)
    {
        dPtr().viewProps = vtkSmartPointer<vtkPropCollection>::New();
    }
    else
    {
        dPtr().viewProps->RemoveAllItems();
    }

    if (dPtr().shouldShowOutline())
    {
        dPtr().viewProps->AddItem(dPtr().outlineActor());
    }

    if (dPtr().shouldShowContent())
    {
        auto subClassProps = fetchViewProps();
        for (subClassProps->InitTraversal(); auto prop = subClassProps->GetNextProp();)
        {
            dPtr().viewProps->AddItem(prop);
        }
    }

    return dPtr().viewProps;
}

vtkAlgorithmOutput * RenderedData::processedOutputPortInternal(unsigned int DEBUG_ONLY(port))
{
    assert(port == 0);

    return transformedCoordinatesOutputPort();
}

void RenderedData::visibilityChangedEvent(bool visible)
{
    AbstractVisualizedData::visibilityChangedEvent(visible);

    if (dPtr().m_outlineActor)
    {
        dPtr().m_outlineActor->SetVisibility(visible
            && (dPtr().representation == Representation::outline
                || dPtr().representation == Representation::both));
    }
}

void RenderedData::representationChangedEvent(Representation /*representation*/)
{
}

void RenderedData::invalidateViewProps()
{
    dPtr().viewPropsInvalid = true;

    emit viewPropCollectionChanged();
}

RenderedData_private & RenderedData::dPtr()
{
    return static_cast<RenderedData_private &>(AbstractVisualizedData::dPtr());
}

const RenderedData_private & RenderedData::dPtr() const
{
    return static_cast<const RenderedData_private &>(AbstractVisualizedData::dPtr());
}
