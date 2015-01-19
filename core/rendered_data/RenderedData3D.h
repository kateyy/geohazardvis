#pragma once

#include <vtkProperty.h>

#include <core/rendered_data/RenderedData.h>


class vtkActorCollection;
class GlyphMapping;


/**
Base class for rendered data represented as vtkActors.
*/
class CORE_API RenderedData3D : public RenderedData
{
    Q_OBJECT

public:
    RenderedData3D(DataObject * dataObject);
    virtual ~RenderedData3D();

    /** VTK view actors (3D view props) visualizing the data object and possibly additional attributes */
    vtkSmartPointer<vtkActorCollection> actors();

    GlyphMapping * glyphMapping();

protected:
    virtual vtkSmartPointer<vtkActorCollection> fetchActors();

    vtkProperty * renderProperty();
    virtual vtkProperty * createDefaultRenderProperty() const;

    void visibilityChangedEvent(bool visible) override;
    virtual void vectorsForSurfaceMappingChangedEvent();

private:
    /** subclasses are supposed to provide 3D view props (vtkActor) */
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override final;

private:
    vtkSmartPointer<vtkProperty> m_renderProperty;

    GlyphMapping * m_vectors;
};
