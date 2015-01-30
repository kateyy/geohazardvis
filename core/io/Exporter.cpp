#include <core/io/Exporter.h>

#include <cassert>

#include <QFileInfo>
#include <QMap>
#include <QString>

#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>

#include <vtkBMPWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkPNGWriter.h>
#include <vtkXMLImageDataWriter.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


bool Exporter::exportData(DataObject * data, const QString & fileName)
{
    if (auto image = dynamic_cast<ImageDataObject *>(data))
        return exportImage(image, fileName);

    if (auto grid = dynamic_cast<VectorGrid3DDataObject *>(data))
        return exportVectorGrid3D(grid, fileName);

    return false;
}

bool Exporter::isExportSupported(DataObject * data)
{
    return formatFilters().contains(data->dataTypeName());
}

QString Exporter::formatFilter(DataObject * data)
{
    return formatFilters().value(data->dataTypeName());
}

const QMap<QString, QString> & Exporter::formatFilters()
{
    static QMap<QString, QString> ff = {
        { ImageDataObject::dataTypeName_s(), "Bitmap (*.bmp);;JPEG (*.jpg  *.jpeg);;PNG (*.png);;" },
        { VectorGrid3DDataObject::dataTypeName_s(), "VTK XML Image Files (*.vti)" }
    };

    return ff;
}

bool Exporter::exportImage(ImageDataObject * image, const QString & fileName)
{
    QString ext = QFileInfo(fileName).suffix().toLower();

    vtkSmartPointer<vtkImageWriter> writer;

    if (ext == "png")
        writer = vtkSmartPointer<vtkPNGWriter>::New();
    else if (ext == "jpg" || ext == "jpeg")
        writer = vtkSmartPointer<vtkJPEGWriter>::New();
    else if (ext == "bmp")
        writer = vtkSmartPointer<vtkBMPWriter>::New();

    assert(writer);

    VTK_CREATE(vtkImageMapToColors, toUChar);
    toUChar->SetInputData(image->dataSet());
    toUChar->SetOutputFormatToLuminance();

    VTK_CREATE(vtkLookupTable, lut);
    const double * range = image->minMaxValue();
    lut->SetTableRange(range[0], range[1]);
    lut->SetNumberOfTableValues(0xFF);
    lut->SetHueRange(0, 0);
    lut->SetSaturationRange(0, 0);
    lut->SetValueRange(0, 1);

    toUChar->SetLookupTable(lut);

    writer->SetInputConnection(toUChar->GetOutputPort());
    writer->SetFileName(fileName.toUtf8().data());
    writer->Write();

    return true;
}

bool Exporter::exportVectorGrid3D(VectorGrid3DDataObject * grid, const QString & fileName)
{
    VTK_CREATE(vtkXMLImageDataWriter, writer);
    writer->SetInputData(grid->dataSet());
    writer->SetFileName(fileName.toUtf8().data());
    writer->SetHeaderTypeToUInt64();
    writer->SetIdTypeToInt64();
    writer->SetCompressorTypeToZLib();
    writer->SetDataModeToAppended();
    writer->EncodeAppendedDataOff();

    return writer->Write() == 1;
}
