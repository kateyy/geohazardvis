#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>

#include <QGridLayout>

#include <vtkCamera.h>
#include <vtkCommand.h>

#include <reflectionzeug/PropertyGroup.h>

#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RendererImplementationPlot.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/propertyguizeug_extension/ColorEditorRGB.h>
#include <gui/widgets/ResidualViewConfigWidget.h>
#include <gui/widgets/CollapsibleGroupBox.h>


using namespace reflectionzeug;


RendererConfigWidget::RendererConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RendererConfigWidget())
    , m_residualUi(nullptr)
    , m_residualGroupBox(nullptr)
    , m_propertyRoot(nullptr)
    , m_currentRenderView(nullptr)
{
    m_ui->setupUi(this);

    connect(m_ui->interactionModeCombo, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
        this, &RendererConfigWidget::setInteractionStyle);

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


    auto residualView = dynamic_cast<ResidualVerificationView *>(renderView);
    if (residualView && !m_residualUi)
    {
        m_residualUi = new ResidualViewConfigWidget();
        m_residualGroupBox = new CollapsibleGroupBox("Residual View");
        auto layout = new QGridLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_residualUi);
        m_residualGroupBox->setLayout(layout);
        m_ui->formLayout_2->addRow(m_residualGroupBox);
    }

    if (m_residualUi)
    {
        m_residualUi->setCurrentView(residualView);
        m_residualUi->setVisible(residualView != nullptr);
    }


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

    // we won't disconnect the following connection, as an old impl may be deleted already when we receive implementationChanged()
    connect(&m_currentRenderView->implementation(), &RendererImplementation::supportedInteractionStrategiesChanged,
        this, &RendererConfigWidget::updateInteractionModeCombo);
    connect(&m_currentRenderView->implementation(), &RendererImplementation::interactionStrategyChanged,
        this, &RendererConfigWidget::updateInteractionModeCombo);

    updateInteractionModeCombo();

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

void RendererConfigWidget::updateInteractionModeCombo()
{
    auto * const combo = m_ui->interactionModeCombo;
    combo->blockSignals(true);

    updatePropertyGroup();

    combo->clear();

    if (!m_currentRenderView)
    {
        combo->blockSignals(false);
        return;
    }

    const auto & impl = m_currentRenderView->implementation();

    combo->addItems(impl.supportedInteractionStrategies());
    combo->setCurrentText(impl.currentInteractionStrategy());

    m_ui->interactionModeWidget->setVisible(combo->count() > 0);

    combo->blockSignals(false);
}

void RendererConfigWidget::updatePropertyGroup()
{
    m_propertyRoot = createPropertyGroup(m_currentRenderView);
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_ui->propertyBrowser->resizeColumnToContents(0);
}

void RendererConfigWidget::setInteractionStyle(const QString & styleName)
{
    assert(m_currentRenderView);

    m_currentRenderView->implementation().setInteractionStrategy(styleName);
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
