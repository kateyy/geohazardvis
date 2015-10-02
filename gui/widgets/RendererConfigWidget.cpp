#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>

#include <vtkAxis.h>
#include <vtkCamera.h>
#include <vtkChartXY.h>
#include <vtkCommand.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkTextProperty.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RendererImplementationPlot.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>


using namespace reflectionzeug;

namespace
{
enum class SelectionMode
{
    none = vtkContextScene::SELECTION_NONE,
    default_ = vtkContextScene::SELECTION_DEFAULT,
    addition = vtkContextScene::SELECTION_ADDITION,
    subtraction = vtkContextScene::SELECTION_SUBTRACTION,
    toggle = vtkContextScene::SELECTION_TOGGLE
};
enum class Notation
{
    standard = vtkAxis::STANDARD_NOTATION,
    scietific = vtkAxis::SCIENTIFIC_NOTATION,
    mixed = vtkAxis::FIXED_NOTATION
};
}


RendererConfigWidget::RendererConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RendererConfigWidget())
    , m_propertyRoot(nullptr)
    , m_currentRenderView(nullptr)
{
    m_ui->setupUi(this);

    m_ui->propertyBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->addPainterPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->setAlwaysExpandGroups(true);

    updateTitle();
}

RendererConfigWidget::~RendererConfigWidget()
{
    clear();
}

void RendererConfigWidget::clear()
{
    m_currentRenderView = nullptr;
    for (auto it = m_cameraObserverTags.begin(); it != m_cameraObserverTags.end(); ++it)
    {
        it.key()->RemoveObserver(it.value());
    }
    m_cameraObserverTags.clear();

    setCurrentRenderView(nullptr);
}

void RendererConfigWidget::setCurrentRenderView(AbstractRenderView * renderView)
{
    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    auto lastRenderView = m_currentRenderView;
    m_currentRenderView = renderView;


    // clear connections with the old view

    if (lastRenderView)
    {
        disconnect(lastRenderView, &AbstractRenderView::windowTitleChanged, this, &RendererConfigWidget::updateTitle);
        disconnect(lastRenderView, &AbstractRenderView::implementationChanged, this, &RendererConfigWidget::updateForNewImplementation);
    }

    RendererImplementationBase3D * impl3D = nullptr;
    if (lastRenderView && (impl3D = dynamic_cast<RendererImplementationBase3D *>(&lastRenderView->implementation())))
    {
        auto camera = impl3D->camera(0);  // assuming synchronized cameras
        auto tag = m_cameraObserverTags.take(camera);
        camera->RemoveObserver(tag);
    }


    // setup for the new view

    if (!m_currentRenderView)
    {
        return;
    }

    connect(m_currentRenderView, &AbstractRenderView::windowTitleChanged, this, &RendererConfigWidget::updateTitle);
    connect(m_currentRenderView, &AbstractRenderView::implementationChanged, this, &RendererConfigWidget::updateForNewImplementation);
    
    m_propertyRoot = createPropertyGroup(m_currentRenderView);
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->resizeColumnToContents(0);

    if (m_currentRenderView && (impl3D = dynamic_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation())))
    {
        auto camera = impl3D->camera(0);  // assuming synchronized cameras
        auto tag = camera->AddObserver(vtkCommand::ModifiedEvent, this, &RendererConfigWidget::readCameraStats);
        m_cameraObserverTags.insert(camera, tag);
    }

    updateTitle();
}

void RendererConfigWidget::updateTitle()
{
    QString title;
    if (m_currentRenderView)
        title = m_currentRenderView->friendlyName();
    else
        title = "(No Render View selected)";

    title = "<b>" + title + "</b>";
    m_ui->relatedRenderView->setText(title);
}

void RendererConfigWidget::updateForNewImplementation()
{
    setCurrentRenderView(m_currentRenderView);
}

PropertyGroup * RendererConfigWidget::createPropertyGroup(AbstractRenderView * renderView)
{
    switch (renderView->contentType())
    {
    case ContentType::Rendered2D:
    case ContentType::Rendered3D:
    {
        RendererImplementationBase3D * impl3D = dynamic_cast<RendererImplementationBase3D *>(&renderView->implementation());
        assert(impl3D);
        return createPropertyGroupRenderer(renderView, impl3D);
    }
    case ContentType::Context2D:
    {
        RendererImplementationPlot * implPlot = dynamic_cast<RendererImplementationPlot *>(&renderView->implementation());
        assert(implPlot);
        return createPropertyGroupPlot(renderView, implPlot);
    }
    default:
        return new PropertyGroup();
    }
}

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupPlot(
    AbstractRenderView * /*renderView*/, RendererImplementationPlot * impl)
{
    PropertyGroup * root = new PropertyGroup();

    root->addProperty<bool>("AxesAutoUpdate", impl, &RendererImplementationPlot::axesAutoUpdate, &RendererImplementationPlot::setAxesAutoUpdate)
        ->setOption("title", "Automatic Axes Update");


    auto addAxisProperties = [impl, root] (int axis, const std::string & groupName, const std::string groupTitle)
    {
        auto group = root->addGroup(groupName);
        group->setOption("title", groupTitle);

        group->addProperty<std::string>("Label",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetTitle(); },
            [impl, axis] (const std::string & label) {
            impl->chart()->GetAxis(axis)->SetTitle(label);
            impl->render();
        });

        auto prop_fontSize = group->addProperty<int>("LabelFontSize",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetTitleProperties()->GetFontSize(); },
            [impl, axis] (int size) {
            impl->chart()->GetAxis(vtkAxis::BOTTOM)->GetTitleProperties()->SetFontSize(size); 
            impl->render();
        });
        prop_fontSize->setOption("title", "Label Font Size");
        prop_fontSize->setOption("minimum", impl->chart()->GetAxis(vtkAxis::BOTTOM)->GetTitleProperties()->GetFontSizeMinValue());
        prop_fontSize->setOption("maximum", impl->chart()->GetAxis(vtkAxis::BOTTOM)->GetTitleProperties()->GetFontSizeMaxValue());

        auto prop_tickFontSize = group->addProperty<int>("TickFontSize",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetLabelProperties()->GetFontSize(); },
            [impl, axis] (int size) {
            impl->chart()->GetAxis(vtkAxis::BOTTOM)->GetLabelProperties()->SetFontSize(size);
            impl->render();
        });
        prop_tickFontSize->setOption("title", "Tick Font Size");
        prop_tickFontSize->setOption("minimum", impl->chart()->GetAxis(vtkAxis::BOTTOM)->GetLabelProperties()->GetFontSizeMinValue());
        prop_tickFontSize->setOption("maximum", impl->chart()->GetAxis(vtkAxis::BOTTOM)->GetLabelProperties()->GetFontSizeMaxValue());

        auto prop_precision = group->addProperty<int>("Precision",
            [impl, axis] () { return impl->chart()->GetAxis(axis)->GetPrecision(); },
            [impl, axis] (int p) {
            impl->chart()->GetAxis(axis)->SetPrecision(p);
            impl->render();
        });
        prop_precision->setOption("minimum", 0);
        prop_precision->setOption("maximum", 100);

        auto prop_notation = group->addProperty<Notation>("Notation",
            [impl, axis] () { return static_cast<Notation>(impl->chart()->GetAxis(axis)->GetNotation()); },
            [impl, axis] (Notation n) { 
            impl->chart()->GetAxis(axis)->SetNotation(static_cast<int>(n));
            impl->render();
        });
        prop_notation->setStrings({
                { Notation::standard, "standard" },
                { Notation::scietific, "scientific" },
                { Notation::mixed, "mixed" }
        });
    };

    addAxisProperties(vtkAxis::BOTTOM, "xAxis", "x-Axis");
    addAxisProperties(vtkAxis::LEFT, "yAxis", "y-Axis");

    root->addProperty<bool>("ChartLegend",
        [impl] () { return impl->chart()->GetShowLegend(); },
        [impl] (bool v) { 
        impl->chart()->SetShowLegend(v);
        impl->render();
    })
        ->setOption("title", "Chart Legend");

    //auto backgroundColor = root->addProperty<Color>("backgroundColor",
    //    [impl] () {
    //    unsigned char * color = impl->chart()->GetBackgroundBrush()->GetColor();
    //    return Color(color[0], color[1], color[2]);
    //},
    //    [impl] (const Color & color) {

    //    impl->chart()->GetBackgroundBrush()->SetColor(color.red(), color.green(), color.blue());
    //    impl->render();
    //});
    //backgroundColor->setOption("title", "Background Color");

    //auto prop_selectionMode = root->addProperty<SelectionMode>("SelectionMode",
    //    [impl] () { return static_cast<SelectionMode>(impl->chart()->GetSelectionMode()); },
    //    [impl] (SelectionMode mode) {
    //    impl->chart()->SetSelectionMode(static_cast<int>(mode));
    //    impl->render();
    //});
    //prop_selectionMode->setStrings({
    //        { SelectionMode::none, "none" },
    //        { SelectionMode::default_, "default" },
    //        { SelectionMode::addition, "addition" },
    //        { SelectionMode::subtraction, "subtraction" },
    //        { SelectionMode::toggle, "toggle" } });

    return root;
}
