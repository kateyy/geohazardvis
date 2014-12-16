#pragma once

#include <vtkProperty.h>

#include <core/rendered_data/RenderedData.h>


class vtkProp3DCollection;
class VectorMapping;


/**
Base class for rendered data represented as vtkActors.
*/
class CORE_API RenderedData3D : public RenderedData
{
    Q_OBJECT

public:
    RenderedData3D(DataObject * dataObject);
    virtual ~RenderedData3D();

    /** VTK 3D view props visualizing the data object and possibly additional attributes */
    vtkSmartPointer<vtkProp3DCollection> viewProps3D();

    VectorMapping * vectorMapping();

protected:
    virtual vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D();

    vtkProperty * renderProperty();
    virtual vtkProperty * createDefaultRenderProperty() const;

    void visibilityChangedEvent(bool visible) override;
    virtual void vectorsForSurfaceMappingChangedEvent();

private:
    /** subclasses are supposed to provide 3D view props (vtkProp3D) */
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override final;

private:
    vtkSmartPointer<vtkProperty> m_renderProperty;

    VectorMapping * m_vectors;
};
