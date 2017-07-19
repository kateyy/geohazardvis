/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScalarBarActor.h"

#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkTextProperty.h>
#include <vtkTextActor.h>
#include <vtkTuple.h>
#include <vtkScalarBarActorInternal.h>


vtkStandardNewMacro(OrientedScalarBarActor);


OrientedScalarBarActor::OrientedScalarBarActor()
    : Superclass()
{
    this->DrawSubTickMarks = 0;

    this->TitleAlignedWithColorBar = true;
    this->TitleTextProperty->SetFontSize(7);
    this->TitleTextProperty->ShadowOff();
    this->TitleTextProperty->SetColor(0, 0, 0);
    this->TitleTextProperty->BoldOff();
    this->TitleTextProperty->ItalicOff();

    this->LabelTextProperty->SetFontSize(6);
    this->LabelTextProperty->ShadowOff();
    this->LabelTextProperty->SetColor(0, 0, 0);
    this->LabelTextProperty->BoldOff();
    this->LabelTextProperty->ItalicOff();

    this->NumberOfLabels = 3;
    this->AddRangeLabels = true;
    this->AddRangeAnnotations = false;
}

void OrientedScalarBarActor::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);

    os << indent << "Title aligned with the color bar: " << this->TitleAlignedWithColorBar << "\n";
}

OrientedScalarBarActor::~OrientedScalarBarActor() = default;

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
