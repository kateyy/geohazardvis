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

#include <cassert>

#include <QDebug>
#include <QScopedPointer>

#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageData.h>
#include <vtkAssignAttribute.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkInteractorStyleTerrain.h>
#include <vtkCamera.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkImageShiftScale.h>
#include <vtkArrayCalculator.h>
#include <vtkImageExtractComponents.h>
#include <vtkExtractVectorComponents.h>

#include <core/vtkhelper.h>
#include <core/vtkcamerahelper.h>
#include <core/io/Loader.h>
#include <core/filters/StackedImageDataLIC3D.h>
#include <core/data_objects/DataObject.h>


int main()
{
    //QString fileName{ "E:/Users/Karsten/Documents/Studium/GFZ/data/VTK XML data/DispVec_Volumetric_5slices.vti" };
    QString fileName{ "E:/Users/Karsten/Documents/Studium/GFZ/data/VTK XML data/DispVec_Volumetric.vti" };

    QScopedPointer<DataObject> data{ Loader::readFile(fileName) };

    vtkSmartPointer<vtkImageData> image = vtkImageData::SafeDownCast(data->dataSet());
 
    VTK_CREATE(vtkRenderer, renderer);
    renderer->SetBackground(1.0, 1.0, 1.0);
    renderer->UseDepthPeelingOn();

    vtkCamera & camera = *renderer->GetActiveCamera();
    camera.SetViewUp(0, 0, 1);
    TerrainCamera::setAzimuth(camera, 0);
    TerrainCamera::setVerticalElevation(camera, 45);
    VTK_CREATE(vtkRenderWindow, window);
    window->AddRenderer(renderer);
    VTK_CREATE(vtkRenderWindowInteractor, interactor);
    VTK_CREATE(vtkInteractorStyleTerrain, style);
    interactor->SetInteractorStyle(style);
    interactor->SetRenderWindow(window);

    VTK_CREATE(vtkArrayCalculator, vectorScale);
    vectorScale = vtkSmartPointer<vtkArrayCalculator>::New();
    vectorScale->SetInputData(image);
    vectorScale->AddVectorArrayName(image->GetPointData()->GetVectors()->GetName());
    vectorScale->SetResultArrayName("scaledVectors");
    std::string fun = "10000*" + std::string{image->GetPointData()->GetVectors()->GetName()};
    vectorScale->SetFunction(fun.c_str());

    VTK_CREATE(vtkAssignAttribute, assignVectors);
    assignVectors->Assign("scaledVectors", vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
    assignVectors->SetInputConnection(vectorScale->GetOutputPort());

    VTK_CREATE(StackedImageDataLIC3D, lic3D);
    lic3D->SetInputConnection(assignVectors->GetOutputPort());

    // make sure to have LIC as sinlge component scalars

    VTK_CREATE(vtkAssignAttribute, toVectors);
    toVectors->SetInputConnection(lic3D->GetOutputPort());
    toVectors->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    VTK_CREATE(vtkExtractVectorComponents, lic3DlicOnly);
    lic3DlicOnly->SetInputConnection(toVectors->GetOutputPort());

    VTK_CREATE(vtkAssignAttribute, toScalars);
    toScalars->SetInputConnection(lic3DlicOnly->GetOutputPort());
    toScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

    //VTK_CREATE(vtkGPUVolumeRayCastMapper, volumeMapper);
    VTK_CREATE(vtkSmartVolumeMapper, volumeMapper);
    //VTK_CREATE(vtkFixedPointVolumeRayCastMapper, volumeMapper);
    volumeMapper->SetInputConnection(lic3DlicOnly->GetOutputPort());
    //volumeMapper->SetRequestedRenderModeToRayCast();

    //volumeMapper->SetBlendModeToMaximumIntensity();
    volumeMapper->SetBlendModeToComposite();
    //volumeMapper->SetBlendModeToMinimumIntensity();

    VTK_CREATE(vtkVolume, volume);
    volume->SetMapper(volumeMapper);

    vtkSmartPointer<vtkVolumeProperty> volumeProperty =
        vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetScalarOpacityUnitDistance(image->GetSpacing()[0]);
    //qDebug() << image->GetSpacing()[0];
    //volumeProperty->SetScalarOpacityUnitDistance(0.001);

    volume->SetProperty(volumeProperty);

    vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity =
        vtkSmartPointer<vtkPiecewiseFunction>::New();
    compositeOpacity->AddPoint(0.0, 0.0);
    compositeOpacity->AddPoint(0.48, 0.0);
    compositeOpacity->AddPoint(0.5, 1.0);
    compositeOpacity->AddPoint(0.52, 0.0);
    compositeOpacity->AddPoint(1.0, 0.0);

    /*compositeOpacity->AddPoint(0.0, 0.0, 0.9, 0.0);
    compositeOpacity->AddPoint(0.5, 1.0, 0.1, 0.0);
    compositeOpacity->AddPoint(1.0, 0.0);*/

    volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

    vtkSmartPointer<vtkColorTransferFunction> color =
        vtkSmartPointer<vtkColorTransferFunction>::New();
    color->SetColorSpaceToRGB();
    color->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
    color->AddRGBPoint(0.45, 0.0, 0.0, 1.0, 0.1, 1.0);
    color->AddRGBPoint(0.5, 1.0, 0.0, 1.0);
    color->AddRGBPoint(0.55, 1.0, 0.0, 0.0);
    color->AddRGBPoint(1.0, 1.0, 0.0, 0.0);

    //color->AddRGBSegment(0, 1, 1, 1, 1, 1, 1, 1);
    volumeProperty->SetColor(color);

    renderer->AddViewProp(volume);

    renderer->ResetCamera();


    interactor->Start();

    return 0;
}