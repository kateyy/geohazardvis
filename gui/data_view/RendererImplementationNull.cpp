#include "RendererImplementationNull.h"

#include <vtkRenderer.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <gui/data_view/t_QVTKWidget.h>


RendererImplementationNull::RendererImplementationNull(AbstractRenderView & renderView)
    : RendererImplementation(renderView)
{
}

QString RendererImplementationNull::name() const
{
    return "NullImplementation";
}

ContentType RendererImplementationNull::contentType() const
{
    return ContentType::invalid;
}

QList<DataObject *> RendererImplementationNull::filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects)
{
    incompatibleObjects = dataObjects;
    return{};
}

void RendererImplementationNull::activate(t_QVTKWidget & qvtkWidget)
{
    if (!m_renderer)
    {
        m_renderer = vtkSmartPointer<vtkRenderer>::New();
        m_renderer->SetBackground(1, 1, 1);
    }

    qvtkWidget.GetRenderWindow()->AddRenderer(m_renderer);
}

void RendererImplementationNull::deactivate(t_QVTKWidget & qvtkWidget)
{
    if (m_renderer)
    {
        qvtkWidget.GetRenderWindow()->RemoveRenderer(m_renderer);
    }
}

void RendererImplementationNull::render()
{
}

vtkRenderWindowInteractor * RendererImplementationNull::interactor()
{
    return nullptr;
}

void RendererImplementationNull::lookAtData(const VisualizationSelection &, unsigned int)
{
}

void RendererImplementationNull::resetCamera(bool, unsigned int)
{
}

void RendererImplementationNull::setAxesVisibility(bool)
{
}

bool RendererImplementationNull::canApplyTo(const QList<DataObject *> &)
{
    return false;
}

std::unique_ptr<AbstractVisualizedData> RendererImplementationNull::requestVisualization(DataObject &) const
{
    return nullptr;
}

void RendererImplementationNull::onAddContent(AbstractVisualizedData *, unsigned int)
{
}

void RendererImplementationNull::onRemoveContent(AbstractVisualizedData *, unsigned int)
{
}

void RendererImplementationNull::onSetSelection(const VisualizationSelection &)
{
}

void RendererImplementationNull::onClearSelection()
{
}
