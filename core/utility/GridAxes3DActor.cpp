#include "GridAxes3DActor.h"

#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTextProperty.h>
#include <vtkVector.h>

#include <core/ThirdParty/ParaView/vtkGridAxes2DActor.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(GridAxes3DActor);


GridAxes3DActor::GridAxes3DActor()
    : Superclass()
    , LabelsVisible{ this->Superclass::GetLabelMask() != 0 }
{
    this->SetFaceMask(255);

    // We need to set a new vtkProperty object here, otherwise the changes will not apply to all
    // faces/axes
    vtkNew<vtkProperty> gridAxesProperty;
    gridAxesProperty->DeepCopy(this->GetProperty());
    gridAxesProperty->BackfaceCullingOff();
    gridAxesProperty->FrontfaceCullingOn();

    this->SetProperty(gridAxesProperty.Get());

    for (int i = 0; i < 3; ++i)
    {
        this->GetLabelTextProperty(i)->SetFontSize(10);
        this->GetLabelTextProperty(i)->SetColor(0, 0, 0);
        this->GetTitleTextProperty(i)->SetFontSize(10);
        this->GetTitleTextProperty(i)->SetColor(0, 0, 0);
    }
}

GridAxes3DActor::~GridAxes3DActor() = default;

void GridAxes3DActor::Update(vtkViewport * viewport)
{
    const unsigned int showLabelsMask = this->LabelsVisible ? 255u : 0u;

    if (auto ren = vtkRenderer::SafeDownCast(viewport))
    {
        /**
         * Faces: Kind of misleading logic. MIN_XZ is actually the face with minimum z
         * coordinates, not with minimal x and y coordinates, etc.
         */
        using Faces = vtkGridAxes2DActor::Faces;

        vtkVector3d pos, focalPoint;
        ren->GetActiveCamera()->GetPosition(pos.GetData());
        ren->GetActiveCamera()->GetFocalPoint(focalPoint.GetData());
        const auto viewDirection = focalPoint - pos;

        if (viewDirection[0] == 0.0 && viewDirection[1] == 0.0)
        {
            // Simple case: image view, parallel projection, with irrelevant z axis.

            this->Superclass::SetLabelMask(LabelMasks::MIN_X | LabelMasks::MIN_Y);
        }
        else
        {
            // Terrain view: visibility of faces and labels depends on the current view orientation.

            // Show x/y axes labels only at one of top or bottom plane, but node at the sides.
            const bool viewFromTop = viewDirection[2] <= 0.0;
            this->GridAxes2DActors[Faces::MAX_XY]->SetLabelMask(viewFromTop ? 0u : showLabelsMask);
            this->GridAxes2DActors[Faces::MIN_XY]->SetLabelMask(viewFromTop ? showLabelsMask : 0u);
            const unsigned int noTopBottom = showLabelsMask & ~(LabelMasks::MAX_Z | LabelMasks::MIN_Z);

            // Always show a pair of x/y planes, but not more. When the view direction is nearly
            // aligned with one of the x/y axes and front face culling is enabled, two opposing
            // side planes are shown (looking "into a tunnel").
            // We always want only one of those to reduce the amount of lines and labels the user
            // is confronted with.
            // min/max x faces
            const bool viewToPosX = viewDirection[0] >= 0.0;
            this->GridAxes2DActors[Faces::MIN_YZ]->SetVisibility(!viewToPosX);
            this->GridAxes2DActors[Faces::MAX_YZ]->SetVisibility(viewToPosX);
            // min/max y faces
            const bool viewToPosY = viewDirection[1] >= 0.0;
            this->GridAxes2DActors[Faces::MIN_ZX]->SetVisibility(!viewToPosY);
            this->GridAxes2DActors[Faces::MAX_ZX]->SetVisibility(viewToPosY);

            // In the terrain view, 3 z axes are always visible, but only the on in the left of the
            // field of view should get labels. That is the one where the respective side plane is
            // not shown. So the z axis in the background that bounds only one visible face gets
            // the labels.
            // All in all, only one of the side planes labels one z axis, all other side planes
            // don't show labels at all.
            int zLabelFaces = -1;
            if (viewToPosX)
            {
                zLabelFaces = viewToPosY ? Faces::MAX_ZX : Faces::MAX_YZ;
            }
            else
            {
                zLabelFaces = viewToPosY ? Faces::MIN_YZ : Faces::MIN_ZX;
            }
            for (int face : { Faces::MIN_YZ, Faces::MIN_ZX, Faces::MAX_YZ, Faces::MAX_ZX })
            {
                this->GridAxes2DActors[face]->SetLabelMask(face == zLabelFaces ? noTopBottom : 0u);
            }
        }
    }
    else
    {
        // Just in case the viewport is not a vtkRenderer for some reason.
        for (int cc = 0; cc < 6; ++cc)
        {
            this->GridAxes2DActors[cc]->SetVisibility(true);
            this->GridAxes2DActors[cc]->SetLabelMask(showLabelsMask);
        }
    }

    Superclass::Update(viewport);
}

void GridAxes3DActor::SetLabelsVisible(const bool visible)
{
    if (visible == this->LabelsVisible)
    {
        return;
    }

    this->LabelsVisible = visible;

    this->Modified();
}

void GridAxes3DActor::SetPrintfAxisLabelFormat(int axis, const vtkStdString & formatString)
{
    for (int cc = 0; cc < 6; cc++)
    {
        this->GridAxes2DActors[cc]->SetPrintfAxisLabelFormat(axis, formatString);
    }
    this->Modified();
}

void GridAxes3DActor::GetEdgeColor(unsigned char edgeColor[3]) const
{
    this->GridAxes2DActors[0]->GetEdgeColor(edgeColor);
}

unsigned char * GridAxes3DActor::GetEdgeColor() const
{
    return this->GridAxes2DActors[0]->GetEdgeColor();
}

void GridAxes3DActor::SetEdgeColor(unsigned char edgeColor[3])
{
    for (int cc = 0; cc < 6; cc++)
    {
        this->GridAxes2DActors[cc]->SetEdgeColor(edgeColor);
    }
    this->Modified();
}

void GridAxes3DActor::SetEdgeColor(unsigned char r, unsigned char g, unsigned char b)
{
    for (int cc = 0; cc < 6; cc++)
    {
        this->GridAxes2DActors[cc]->SetEdgeColor(r, g, b);
    }
    this->Modified();
}

void GridAxes3DActor::GetGridLineColor(unsigned char gridLineColor[3]) const
{
    this->GridAxes2DActors[0]->GetGridLineColor(gridLineColor);
}

unsigned char * GridAxes3DActor::GetGridLineColor() const
{
    return this->GridAxes2DActors[0]->GetGridLineColor();
}

void GridAxes3DActor::SetGridLineColor(unsigned char gridLineColor[3])
{
    for (int cc = 0; cc < 6; cc++)
    {
        this->GridAxes2DActors[cc]->SetGridLineColor(gridLineColor);
    }
    this->Modified();
}

void GridAxes3DActor::SetGridLineColor(unsigned char r, unsigned char g, unsigned char b)
{
    for (int cc = 0; cc < 6; cc++)
    {
        this->GridAxes2DActors[cc]->SetGridLineColor(r, g, b);
    }
    this->Modified();
}

void GridAxes3DActor::SetLabelMask(const unsigned int /*mask*/)
{
    // Not valid for this subclass.
}
