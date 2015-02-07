#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>

#include <vtkBrush.h>
#include <vtkCamera.h>
#include <vtkCollection.h>
#include <vtkTextProperty.h>
#include <vtkEventQtSlotConnect.h>

#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/types.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementation3D.h>
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
    , m_eventConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
    , m_eventEmitters(vtkSmartPointer<vtkCollection>::New())
{
    m_ui->setupUi(this);

    m_ui->propertyBrowser->addEditorPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->addPainterPlugin<ColorEditorRGB>();
    m_ui->propertyBrowser->setAlwaysExpandGroups(true);

    connect(m_ui->relatedRenderer, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, static_cast<void(RendererConfigWidget::*)(int)>(&RendererConfigWidget::setCurrentRenderView));
}

RendererConfigWidget::~RendererConfigWidget()
{
    clear();
    delete m_ui;
}

void RendererConfigWidget::clear()
{
    m_ui->relatedRenderer->clear();

    m_currentRenderView = nullptr;
    m_eventConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New(); // recreate, discarding all previous connections
    m_eventEmitters->RemoveAllItems();

    setCurrentRenderView(-1);
}

void RendererConfigWidget::setRenderViews(const QList<RenderView *> & renderViews)
{
    clear();

    for (RenderView * renderView : renderViews)
    {
        m_ui->relatedRenderer->addItem(
            renderView->friendlyName(),
            reinterpret_cast<qulonglong>(renderView));

        connect(renderView, &RenderView::windowTitleChanged, this, &RendererConfigWidget::updateRenderViewTitle);
    }

    if (renderViews.isEmpty())
        return;
}

void RendererConfigWidget::setCurrentRenderView(RenderView * renderView)
{
    if (renderView)
        m_ui->relatedRenderer->setCurrentText(renderView->friendlyName());
}

void RendererConfigWidget::setCurrentRenderView(int index)
{
    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    if (index < 0)
        return;

    RenderView * lastRenderView = m_currentRenderView;
    m_currentRenderView = reinterpret_cast<RenderView *>(
        m_ui->relatedRenderer->itemData(index, Qt::UserRole).toULongLong());
    assert(m_currentRenderView);

    m_propertyRoot = createPropertyGroup(m_currentRenderView);
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->setColumnWidth(0, 135);


    RendererImplementation3D * impl3D;
    if (lastRenderView && (impl3D = dynamic_cast<RendererImplementation3D *>(&lastRenderView->implementation())))
    {
        m_eventConnect->Disconnect(impl3D->camera(), vtkCommand::ModifiedEvent, this, SLOT(readCameraStats(vtkObject *)), this);
        m_eventEmitters->RemoveItem(impl3D->camera());
    }
    if (m_currentRenderView && (impl3D = dynamic_cast<RendererImplementation3D *>(&m_currentRenderView->implementation())))
    {
        m_eventConnect->Connect(impl3D->camera(), vtkCommand::ModifiedEvent, this, SLOT(readCameraStats(vtkObject *)), this);
        m_eventEmitters->AddItem(impl3D->camera());
    }
}

void RendererConfigWidget::updateRenderViewTitle(const QString & newTitle)
{
    RenderView * renderView = dynamic_cast<RenderView *>(sender());
    assert(renderView);
    qulonglong ptr = reinterpret_cast<qulonglong>(renderView);

    for (int i = 0; i < m_ui->relatedRenderer->count(); ++i)
    {
        if (m_ui->relatedRenderer->itemData(i, Qt::UserRole).toULongLong() == ptr)
        {
            m_ui->relatedRenderer->setItemText(i, newTitle);
            setCurrentRenderView(i);
            return;
        }
    }
}

PropertyGroup * RendererConfigWidget::createPropertyGroup(RenderView * renderView)
{
    switch (renderView->contentType())
    {
    case ContentType::Rendered2D:
    case ContentType::Rendered3D:
    {
        RendererImplementation3D * impl3D = dynamic_cast<RendererImplementation3D *>(&renderView->implementation());
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

reflectionzeug::PropertyGroup * RendererConfigWidget::createPropertyGroupPlot(RenderView * /*renderView*/, RendererImplementationPlot * impl)
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
