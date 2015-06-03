#pragma once

#include <core/rendered_data/RenderedData.h>


class vtkProp3DCollection;
class vtkProperty;

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

    /** VTK 3D view props visualizing the data object and possibly additional attributes */
    vtkSmartPointer<vtkProp3DCollection> viewProps3D();

    GlyphMapping * glyphMapping();

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

    GlyphMapping * m_glyphMapping;
};
