#include "ScalarBarActor.h"

#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkTextProperty.h>
#include <vtkTextActor.h>
#include <vtkTuple.h>
#include <vtkScalarBarActorInternal.h>


vtkStandardNewMacro(OrientedScalarBarActor);


OrientedScalarBarActor::OrientedScalarBarActor()
{
    this->TitleAlignedWithColorBar = true;
    this->TitleTextProperty->SetFontSize(5);
    this->TitleTextProperty->ShadowOff();
    this->TitleTextProperty->SetColor(0, 0, 0);
    this->TitleTextProperty->BoldOff();
    this->TitleTextProperty->ItalicOff();

    //m_colorMappingLegend->SetNumberOfLabels(7);

    this->LabelTextProperty->SetFontSize(5);
    this->LabelTextProperty->ShadowOff();
    this->LabelTextProperty->SetColor(0, 0, 0);
    this->LabelTextProperty->BoldOff();
    this->LabelTextProperty->ItalicOff();
}

void OrientedScalarBarActor::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);

    os << indent << "Title aligned with the color bar: " << this->TitleAlignedWithColorBar << "\n";
}

void OrientedScalarBarActor::LayoutTitle()
{
    this->Superclass::LayoutTitle();

    this->TitleActor->GetTextProperty()->SetOrientation(
        this->TitleAlignedWithColorBar && this->Orientation == VTK_ORIENT_VERTICAL
        ? 90.0 : 0.0);
}

void OrientedScalarBarActor::ConfigureTitle()
{  
    // partly copied from vtkPVScalarBarActor::ConfigureTitle, but adjusted for the title aligned with the color bar
 
    if (!this->TitleAlignedWithColorBar || this->Orientation == VTK_ORIENT_HORIZONTAL)
    {
        return this->Superclass::ConfigureTitle();
    }

    double texturePolyDataBounds[6];
    this->TexturePolyData->GetBounds(texturePolyDataBounds);

    double scalarBarMinX = texturePolyDataBounds[0];
    double scalarBarMinY = texturePolyDataBounds[2];
    double scalarBarMaxY = texturePolyDataBounds[3];
    double x, y;

    this->TitleActor->GetTextProperty()->SetJustification(VTK_TEXT_CENTERED);
    x = scalarBarMinX;
    y = this->P->TitleBox.Posn[1] - 0.5 * (scalarBarMinY + scalarBarMaxY);

    this->TitleActor->SetPosition(x, y);
}
