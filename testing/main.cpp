
#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkAssignAttribute.h>
#include <vtkExtractVOI.h>
#include <vtkLookupTable.h>
#include <vtkTexture.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

//#include <vtkSmartVolumeMapper.h>
//#include <vtkVolume.h>
//#include <vtkVolumeProperty.h>
//#include <vtkColorTransferFunction.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <core/Loader.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


#define VTK_CREATE(CLASS, NAME) vtkSmartPointer<CLASS> NAME = vtkSmartPointer<CLASS>::New();

int main()
{

    QString textFile = "D:/Documents/Studium/GFZ/data/dataSets_meta_2/DispVec_Volumetric_5slices.txt";
    //QString textFile = "D:/Documents/Studium/GFZ/data/dataSets_meta_2/DispVec_Volumetric.txt";
    QFileInfo textFileInfo(textFile);
    QString rawFile = textFileInfo.path() + "/" + textFileInfo.baseName() + ".raw";

    DataObject * obj = Loader::readFile(textFile);

    //DataObject * obj = Loader::readFile("D:/Documents/Studium/GFZ/data/dataSets_meta_2/displacements.txt");
    VectorGrid3DDataObject * grid = dynamic_cast<VectorGrid3DDataObject *>(obj);
    if (!grid)
        return 1;

    vtkImageData * image = (vtkImageData*)grid->dataSet();

    vtkSmartPointer<vtkFloatArray> data = vtkFloatArray::SafeDownCast(image->GetPointData()->GetVectors());

    //QFile rawOutFile(rawFile);
    //rawOutFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    //rawOutFile.write((char*)data->GetPointer(0), data->GetSize() * data->GetDataTypeSize());
    //rawOutFile.close();

    //return 0;

    image->GetPointData()->RemoveArray(0);
    data = nullptr;

    QFile rawInFile(rawFile);
    size_t fileSize = rawInFile.size();

    data = vtkSmartPointer<vtkFloatArray>::New();
    data->SetNumberOfComponents(3);
    vtkIdType arraySize = fileSize / 3 / data->GetDataTypeSize();
    data->SetNumberOfTuples(arraySize);
    rawInFile.read((char*)data->GetPointer(0), fileSize);

    image->GetPointData()->SetVectors(data);

    VTK_CREATE(vtkAssignAttribute, assignVectorToScalars);
    assignVectorToScalars->SetInputData(image);
    assignVectorToScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

    VTK_CREATE(vtkExtractVOI, extractSlice);
    extractSlice->SetInputConnection(assignVectorToScalars->GetOutputPort());
    
    int voi[6];
    image->GetExtent(voi);
    voi[4] = voi[5] = (voi[5] - voi[4]) / 2;
    extractSlice->SetVOI(voi);

    extractSlice->Update();
    vtkImageData * slice = extractSlice->GetOutput();
    //vtkImageData * slice = vtkImageData::SafeDownCast(obj->dataSet());

    VTK_CREATE(vtkLookupTable, lut);
    lut->SetTableRange(slice->GetScalarRange());
    qDebug() << "scalar range: " << slice->GetScalarRange()[0] << slice->GetScalarRange()[1];
    lut->Build();

    VTK_CREATE(vtkTexture, texture);
    texture->SetLookupTable(lut);
    texture->SetInputData(slice);
    texture->MapColorScalarsThroughLookupTableOn();
    texture->InterpolateOn();

    const double * extent = slice->GetBounds();
    double xMin = extent[0], xMax = extent[1], yMin = extent[2], yMax = extent[3], zMin = extent[4], zMax = extent[5];

    qDebug() << "slice/plane dimensions: " << QString::number(slice->GetDimensions()[0]) << QString::number(slice->GetDimensions()[1]);
    VTK_CREATE(vtkPlaneSource, plane);
    plane->SetXResolution(slice->GetDimensions()[0]);
    plane->SetYResolution(slice->GetDimensions()[1]);
    plane->SetOrigin(xMin, yMin, zMin);
    // zSlice: xy-Plane
    plane->SetPoint1(xMax, yMin, zMin);
    plane->SetPoint2(xMin, yMax, zMin);

    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(plane->GetOutputPort());

    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(mapper);
    actor->SetTexture(texture);


    /*VTK_CREATE(vtkSmartVolumeMapper, volumeMapper);
    volumeMapper->SetInputConnection(obj->processedOutputPort());
    volumeMapper->SetScalarModeToUsePointData();
    volumeMapper->SelectScalarArray(0);
    VTK_CREATE(vtkColorTransferFunction, colorTransfer);
    colorTransfer->AddRGBSegment(
        obj->processedDataSet()->GetScalarRange()[0], 0, 0, 0,
        obj->processedDataSet()->GetScalarRange()[1], 1, 0, 1);
    VTK_CREATE(vtkVolumeProperty, volumeProp);
    volumeProp->SetColor(colorTransfer);
    VTK_CREATE(vtkVolume, volume);
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProp);*/

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor);
    //renderer->AddViewProp(volume);
    renderer->SetBackground(0.99, 0.99, 1);

    VTK_CREATE(vtkRenderWindow, rw);
    rw->AddRenderer(renderer);

    VTK_CREATE(vtkRenderWindowInteractor, interactor);
    rw->SetInteractor(interactor);
    interactor->Initialize();

    interactor->Start();

    delete obj;

    return 0;
}
