#include "RendererConfigWidget.h"
#include "ui_RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <vtkRenderer.h>
#include <vtkLightKit.h>
#include <vtkCamera.h>

#include <vtkCallbackCommand.h>
#include <vtkObjectFactory.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkcamerahelper.h>
#include "gui/data_view/RenderView.h"


using namespace reflectionzeug;
using namespace propertyguizeug;


struct CameraCallbackCommand : public vtkCallbackCommand
{
    static CameraCallbackCommand * New();
    vtkTypeMacro(CameraCallbackCommand, vtkCallbackCommand);
    void Execute(vtkObject * caller, unsigned long /*eid*/, void * /*dcallData*/) override
    {
        assert(rendererConfigWidget);
        if (caller == rendererConfigWidget->m_activeCamera)
            rendererConfigWidget->activeCameraChangedEvent();
    }

    RendererConfigWidget * rendererConfigWidget = nullptr;
};

vtkStandardNewMacro(CameraCallbackCommand);


RendererConfigWidget::RendererConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RendererConfigWidget())
    , m_propertyRoot(nullptr)
    , m_activeCamera(nullptr)
{
    m_ui->setupUi(this);

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

    setCurrentRenderView(-1);
}

void RendererConfigWidget::setRenderViews(const QList<RenderView *> & renderViews)
{
    clear();

    for (RenderView * renderView : renderViews)
    {
        m_ui->relatedRenderer->addItem(
            renderView->friendlyName(),
            reinterpret_cast<size_t>(renderView));

        connect(renderView, &RenderView::windowTitleChanged, this, &RendererConfigWidget::updateRenderViewTitle);

        vtkSmartPointer<CameraCallbackCommand> callbackCommand = vtkSmartPointer<CameraCallbackCommand>::New();
        callbackCommand->rendererConfigWidget = this;
        renderView->renderer()->GetActiveCamera()->AddObserver(vtkCommand::ModifiedEvent, callbackCommand);
    }

    if (renderViews.isEmpty())
        return;
}

void RendererConfigWidget::setCurrentRenderView(RenderView * renderView)
{
    m_ui->relatedRenderer->setCurrentText(renderView->friendlyName());
}

void RendererConfigWidget::setCurrentRenderView(int index)
{
    m_ui->propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    if (index < 0)
        return;

    RenderView * renderView = reinterpret_cast<RenderView *>(
        m_ui->relatedRenderer->itemData(index, Qt::UserRole).toULongLong());
    assert(renderView);

    m_propertyRoot = createPropertyGroup(renderView);
    m_ui->propertyBrowser->setRoot(m_propertyRoot);
    m_activeCamera = renderView->renderer()->GetActiveCamera();
}

void RendererConfigWidget::updateRenderViewTitle(const QString & newTitle)
{
    RenderView * renderView = dynamic_cast<RenderView *>(sender());
    assert(renderView);
    size_t ptr = reinterpret_cast<size_t>(renderView);

    for (int i = 0; i < m_ui->relatedRenderer->count(); ++i)
    {
        if (m_ui->relatedRenderer->itemData(i, Qt::UserRole).toULongLong() == ptr)
        {
            m_ui->relatedRenderer->setItemText(i, newTitle);
            return;
        }
    }
}

PropertyGroup * RendererConfigWidget::createPropertyGroup(RenderView * renderView)
{
    PropertyGroup * root = new PropertyGroup();

    auto backgroundColor = root->addProperty<Color>("backgroundColor",
        [renderView]() {
        double * color = renderView->renderer()->GetBackground();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderView](const Color & color) {
        renderView->renderer()->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        renderView->render();
    });
    backgroundColor->setOption("title", "background color");

    auto cameraGroup = root->addGroup("Camera");
    {
        vtkCamera * camera = renderView->renderer()->GetActiveCamera();
        auto prop_azimuth = cameraGroup->addProperty<double>("azimuth",
            [camera]() {
            return TerrainCamera::getAzimuth(camera);
        },
            [renderView, camera](const double & azimuth) {
            TerrainCamera::setAzimuth(camera, azimuth);
            renderView->render();
        });
        prop_azimuth->setOption("minimum", std::numeric_limits<double>::lowest());
    }

    PropertyGroup * lightingGroup = root->addGroup("Lighting");

    auto prop_intensity = lightingGroup->addProperty<double>("intensity",
        [renderView]() { return renderView->lightKit()->GetKeyLightIntensity(); },
        [renderView](double value) {
        renderView->lightKit()->SetKeyLightIntensity(value);
        renderView->render();
    });
    prop_intensity->setOption("minimum", 0);
    prop_intensity->setOption("maximum", 3);
    prop_intensity->setOption("step", 0.05);

    return root;
}

void RendererConfigWidget::activeCameraChangedEvent()
{
    m_propertyRoot->property("Camera/azimuth")->asValue()->valueChanged();
}
