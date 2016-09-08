#pragma once

#include <memory>

#include <core/rendered_data/RenderedData.h>


class vtkProp3DCollection;
class vtkProperty;

class GlyphMapping;


/**
Base class for rendered data represented as vtkActors.
*/
class CORE_API RenderedData3D : public RenderedData
{
public:
    explicit RenderedData3D(DataObject & dataObject);
    ~RenderedData3D() override;

    /** VTK 3D view props visualizing the data object and possibly additional attributes */
    vtkSmartPointer<vtkProp3DCollection> viewProps3D();

    vtkProperty * renderProperty();
    virtual vtkSmartPointer<vtkProperty> createDefaultRenderProperty() const;

    GlyphMapping & glyphMapping();

protected:
    virtual vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D();

    void visibilityChangedEvent(bool visible) override;
    virtual void vectorsForSurfaceMappingChangedEvent();

private:
    /** subclasses are supposed to provide 3D view props (vtkProp3D) */
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override final;

private:
    vtkSmartPointer<vtkProperty> m_renderProperty;

    std::unique_ptr<GlyphMapping> m_glyphMapping;

private:
    Q_DISABLE_COPY(RenderedData3D)
};
