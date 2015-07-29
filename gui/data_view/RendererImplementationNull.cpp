#include "RendererImplementationNull.h"

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <QVTKWidget.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>


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

void RendererImplementationNull::activate(QVTKWidget * qvtkWidget)
{
    if (!m_renderer)
    {
        m_renderer = vtkSmartPointer<vtkRenderer>::New();
        m_renderer->SetBackground(1, 1, 1);
    }

    qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);
}

void RendererImplementationNull::deactivate(QVTKWidget * qvtkWidget)
{
    if (m_renderer)
        qvtkWidget->GetRenderWindow()->RemoveRenderer(m_renderer);
}

void RendererImplementationNull::render()
{
}

vtkRenderWindowInteractor * RendererImplementationNull::interactor()
{
    return nullptr;
}

void RendererImplementationNull::setSelectedData(DataObject *, vtkIdType /*= -1*/)
{
}

DataObject * RendererImplementationNull::selectedData() const
{
    return nullptr;
}

vtkIdType RendererImplementationNull::selectedIndex() const
{
    return -1;
}

void RendererImplementationNull::lookAtData(DataObject *, vtkIdType)
{
}

void RendererImplementationNull::resetCamera(bool)
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
