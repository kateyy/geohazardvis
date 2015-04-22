#include "RendererImplementation3D.h"

#include <cassert>

#include <vtkRenderWindow.h>
#include <QVTKWidget.h>

#include <core/color_mapping/ColorMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RenderViewStrategy.h>
#include <gui/data_view/RenderViewStrategySwitch.h>
#include <gui/rendering_interaction/IPickingInteractorStyle.h>


bool RendererImplementation3D::s_isRegistered = RendererImplementation::registerImplementation<RendererImplementation3D>();


RendererImplementation3D::RendererImplementation3D(AbstractRenderView & renderView, QObject * parent)
    : RendererImplementationBase3D(renderView, new ColorMapping(), vtkSmartPointer<vtkRenderWindow>::New(), parent)
    , m_strategySwitch(new RenderViewStrategySwitch(*this))
{
    connect(&m_renderView, &AbstractRenderView::visualizationsChanged, 
        this, &RendererImplementation3D::updateColorMapping);
}

RendererImplementation3D::~RendererImplementation3D()
{
    delete m_strategySwitch;
    delete colorMapping();
}


QList<DataObject *> RendererImplementation3D::filterCompatibleObjects(
    const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects)
{
    if (!m_strategy)
        emit resetStrategy(dataObjects);

    return strategy().filterCompatibleObjects(dataObjects, incompatibleObjects);
}

void RendererImplementation3D::activate(QVTKWidget * qvtkWidget)
{
    // make sure to reuse the existing render window interactor
    renderWindow()->SetInteractor(qvtkWidget->GetInteractor());

    // this will initialize basic rendering features
    RendererImplementationBase3D::activate(qvtkWidget);

    // we created the render window, so pass it to the qvtkWidget
    qvtkWidget->SetRenderWindow(renderWindow());
}

void RendererImplementation3D::deactivate(QVTKWidget * qvtkWidget)
{
    // this is our render window, so remove it from the widget
    qvtkWidget->SetRenderWindow(nullptr);
    // the interactor belongs to the widget, we should reference it anymore
    renderWindow()->SetInteractor(nullptr);

    RendererImplementationBase3D::deactivate(qvtkWidget);
}

void RendererImplementation3D::updateColorMapping()
{
    colorMapping()->setVisualizedData(m_renderView.visualizations());
    if (interactorStyle())
        interactorStyle()->setRenderedData(renderedData());
}
