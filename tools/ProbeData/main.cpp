#include <cassert>

#include <QDebug>

#include <vtkPolyData.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>

#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkDelaunay2D.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>
#include <vtkAssignAttribute.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/io/Exporter.h>
#include <core/io/FileParser.h>
#include <core/io/Loader.h>
#include <core/io/MatricesToVtk.h>
#include <core/io/TextFileReader.h>


int main()
{
    QString fileName{ "C:/develop/$sync/GFZ/data/_original/003 EnvirVis paper/Lazufre/ModellingMovie.txt" };
    QString highResMeshFN{ "C:/develop/$sync/GFZ/data/VTK XML data/Volcano High Res topo.vtp" };

    QString exportFN{ "C:/develop/$sync/GFZ/data/Lazufre_movie.vtp" };

    auto highResMeshData = Loader::readFile(highResMeshFN);
    vtkSmartPointer<vtkDataSet> highResMesh = highResMeshData->dataSet();
    highResMeshData.reset();

    std::vector<std::vector<io::t_FP>> raw_pointData;
    FileParser::populateIOVectors(fileName.toStdString(), raw_pointData);
    vtkIdType numLazufrePoints = raw_pointData[0].size();


    auto points = vtkSmartPointer<vtkPoints>::New();
    auto verts = vtkSmartPointer<vtkCellArray>::New();
    std::vector<vtkIdType> pointIds(numLazufrePoints);
    points->SetNumberOfPoints(numLazufrePoints);
    for (vtkIdType i = 0; i < numLazufrePoints; ++i)
    {
        points->SetPoint(i, raw_pointData[1][i], raw_pointData[2][i], raw_pointData[3][i]);
        pointIds[i] = i;
    }
    verts->InsertNextCell(numLazufrePoints, pointIds.data());

    auto lazufrePoints = vtkSmartPointer<vtkPolyData>::New();
    lazufrePoints->SetPoints(points);
    lazufrePoints->SetVerts(verts);


    auto lazufreElevation = vtkSmartPointer<vtkFloatArray>::New();
    lazufreElevation->SetName("elevation");
    lazufreElevation->SetNumberOfComponents(1);
    lazufreElevation->SetNumberOfValues(numLazufrePoints);
    lazufreElevation->SetNumberOfValues(numLazufrePoints);
    for (vtkIdType i = 0; i < numLazufrePoints; ++i)
        lazufreElevation->SetValue(i, static_cast<float>(raw_pointData[3][i])); // z coords

    lazufrePoints->GetPointData()->AddArray(lazufreElevation);

    vtkIdType firstAttr = 4;
    vtkIdType lastAttr = raw_pointData.size() - 1;
    for (vtkIdType a = firstAttr; a <= lastAttr; ++a)
    {
        auto attrData = vtkSmartPointer<vtkFloatArray>::New();
        QString n = "line of sight (km) " + QString::number(a + 1 - firstAttr);
        attrData->SetName(n.toLatin1().data());
        attrData->SetNumberOfComponents(1);
        attrData->SetNumberOfValues(numLazufrePoints);
        for (vtkIdType i = 0; i < numLazufrePoints; ++i)
            attrData->SetValue(i, static_cast<float>(raw_pointData[a][i]));

        lazufrePoints->GetPointData()->AddArray(attrData);
    }


    auto delaunay = vtkSmartPointer<vtkDelaunay2D>::New(); // probe won't work on simple point set
    delaunay->SetInputData(lazufrePoints);

    auto lazufreFlattener = vtkSmartPointer<vtkTransform>::New();
    lazufreFlattener->Scale(1, 1, 0);
    auto lazufreFlattenerFilter = vtkSmartPointer<vtkTransformFilter>::New();
    lazufreFlattenerFilter->SetTransform(lazufreFlattener);
    //lazufreFlattenerFilter->SetInputData(lazufrePoints);
    lazufreFlattenerFilter->SetInputConnection(delaunay->GetOutputPort());

    lazufreFlattenerFilter->Update();
    lazufreFlattenerFilter->GetOutput()->Print(std::cout);


    double lazufreBounds[6];    // target bounds
    lazufrePoints->GetBounds(lazufreBounds);
    double meshBounds[6];       // to be adjusted
    highResMesh->GetBounds(meshBounds);

    double lazufreCenter[2] = { 0.5*(lazufreBounds[0] + lazufreBounds[1]), 0.5*(lazufreBounds[2] + lazufreBounds[3]) };
    double meshCenter[2] = { 0.5*(meshBounds[0] + meshBounds[1]), 0.5*(meshBounds[2] + meshBounds[3]) };
    double lazufreSize[2] = { lazufreBounds[1] - lazufreBounds[0], lazufreBounds[3] - lazufreBounds[2] };
    double meshSize[2] = { meshBounds[1] - meshBounds[0], meshBounds[3] - meshBounds[2] };


    double scaleEpsilon = 0.95;  // better a bit smaller, to prevent artifacts at data set borders

    auto meshToPointsTransform = vtkSmartPointer<vtkTransform>::New();
    meshToPointsTransform->PostMultiply();
    meshToPointsTransform->Translate(-meshCenter[0], -meshCenter[1], 0);
    meshToPointsTransform->Scale(scaleEpsilon * lazufreSize[0] / meshSize[0], scaleEpsilon * lazufreSize[1] / meshSize[1], 0); // also flattening
    meshToPointsTransform->Translate(lazufreCenter[0], lazufreCenter[1], 0);

    auto meshToPointsTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
    meshToPointsTransformFilter->SetTransform(meshToPointsTransform);
    meshToPointsTransformFilter->SetInputData(highResMesh);

    meshToPointsTransformFilter->Update();
    meshToPointsTransformFilter->GetOutput()->GetBounds(meshBounds);

    lazufreFlattenerFilter->Update();
    lazufreFlattenerFilter->GetOutput()->GetBounds(lazufreBounds);

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(meshToPointsTransformFilter->GetOutputPort());
    probe->SetSourceConnection(lazufreFlattenerFilter->GetOutputPort());
    probe->PassCellArraysOn();
    probe->PassPointArraysOn();
    probe->PassFieldArraysOn();

    probe->Update();
    probe->GetOutput()->Print(std::cout);
    qDebug() << "Valid probe points: " << probe->GetValidPoints()->GetNumberOfTuples();


    auto assignHeightsToScalars = vtkSmartPointer<vtkAssignAttribute>::New();
    assignHeightsToScalars->Assign("elevation", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    assignHeightsToScalars->SetInputConnection(probe->GetOutputPort());

    auto warpElevation = vtkSmartPointer<vtkWarpScalar>::New();
    warpElevation->SetInputConnection(assignHeightsToScalars->GetOutputPort());

    warpElevation->Update();

    vtkSmartPointer<vtkPolyData> LazufreMovie = vtkPolyData::SafeDownCast(warpElevation->GetOutput());
    LazufreMovie->GetPointData()->RemoveArray("elevation");
    LazufreMovie->GetPointData()->RemoveArray("vtkValidPointMask");

    LazufreMovie->Print(std::cout);

    PolyDataObject lazufrePolyData("Lazufre", *LazufreMovie);
    Exporter::exportData(&lazufrePolyData, exportFN);


    auto window = vtkSmartPointer<vtkRenderWindow>::New();
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(window);
    window->SetInteractor(interactor);

    auto ren = vtkSmartPointer<vtkRenderer>::New();
    ren->SetBackground(1, 1, 1);
    window->AddRenderer(ren);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(vtkPolyData::SafeDownCast(LazufreMovie));

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1, 0, 0);
    actor->GetProperty()->SetEdgeColor(0, 1, 0);
    actor->GetProperty()->EdgeVisibilityOn();

    ren->AddViewProp(actor);
    ren->ResetCamera();

    interactor->Initialize();
    interactor->Start();

    return 0;
}
