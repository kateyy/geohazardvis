#include "RenderedData.h"

#include <cassert>

#include <vtkActor.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>

#include <reflectionzeug/Property.h>
#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/DataObject.h>


using namespace reflectionzeug;


RenderedData::RenderedData(ContentType contentType, DataObject & dataObject)
    : AbstractVisualizedData(contentType, dataObject)
    , m_viewPropsInvalid{ true }
    , m_representation{ Representation::content }
    , m_outlineProperty{ vtkSmartPointer<vtkProperty>::New() }
{
    const auto darkGray = 0.2;
    m_outlineProperty->SetColor(darkGray, darkGray, darkGray);
}

RenderedData::~RenderedData() = default;

RenderedData::Representation RenderedData::representation() const
{
    return m_representation;
}

void RenderedData::setRepresentation(Representation representation)
{
    if (m_representation == representation)
    {
        return;
    }

    m_representation = representation;

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
        return static_cast<unsigned>(m_outlineProperty->GetLineWidth());
    },
        [this] (unsigned width) {
        m_outlineProperty->SetLineWidth(static_cast<float>(width));
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
        double * color = m_outlineProperty->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this] (const Color & color) {
        m_outlineProperty->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    })
        ->setOption("title", "Outline Color");

    return group;
}

vtkSmartPointer<vtkPropCollection> RenderedData::viewProps()
{
    if (!m_viewPropsInvalid)
    {
        assert(m_viewProps);
        return m_viewProps;
    }

    m_viewPropsInvalid = false;

    if (!m_viewProps)
    {
        m_viewProps = vtkSmartPointer<vtkPropCollection>::New();
    }
    else
    {
        m_viewProps->RemoveAllItems();
    }

    if (shouldShowOutline())
    {
        m_viewProps->AddItem(outlineActor());
    }

    if (shouldShowContent())
    {
        auto subClassProps = fetchViewProps();
        for (subClassProps->InitTraversal(); auto prop = subClassProps->GetNextProp();)
        {
            m_viewProps->AddItem(prop);
        }
    }

    return m_viewProps;
}

void RenderedData::visibilityChangedEvent(bool visible)
{
    AbstractVisualizedData::visibilityChangedEvent(visible);

    if (m_outlineActor)
    {
        m_outlineActor->SetVisibility(visible 
            && (m_representation == Representation::outline || m_representation == Representation::both));
    }
}

void RenderedData::representationChangedEvent(Representation /*representation*/)
{
}

void RenderedData::invalidateViewProps()
{
    m_viewPropsInvalid = true;

    emit viewPropCollectionChanged();
}

bool RenderedData::shouldShowContent() const
{
    return isVisible()
        && (m_representation == Representation::content
            || m_representation == Representation::both);
}

bool RenderedData::shouldShowOutline() const
{
    return isVisible()
        && (m_representation == Representation::outline
            || m_representation == Representation::both);
}

vtkActor * RenderedData::outlineActor()
{
    if (m_outlineActor)
    {
        return m_outlineActor;
    }

    auto outline = vtkSmartPointer<vtkOutlineFilter>::New();
    outline->SetInputConnection(dataObject().processedOutputPort());

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(outline->GetOutputPort());

    setupInformation(*mapper->GetInformation(), *this);

    m_outlineActor = vtkSmartPointer<vtkActor>::New();
    m_outlineActor->SetMapper(mapper);
    m_outlineActor->SetProperty(m_outlineProperty);

    return m_outlineActor;
}
