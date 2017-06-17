#include <core/io/Exporter.h>

#include <cassert>
#include <limits>

#include <QFileInfo>
#include <QMap>
#include <QString>

#include <vtkBMPWriter.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkJPEGWriter.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPNGWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/filters/ImageMapToColors.h>
#include <core/utility/DataExtent.h>


namespace
{

bool writeVTKXML(vtkXMLWriter * writer, const QString & fileName)
{
    writer->SetFileName(fileName.toUtf8().data());
    writer->SetHeaderTypeToUInt64();
    writer->SetIdTypeToInt64();
    writer->SetCompressorTypeToZLib();
    writer->SetDataModeToAppended();
    writer->EncodeAppendedDataOff();

    return writer->Write() == 1;
}

}


bool Exporter::exportData(DataObject & data, const QString & fileName)
{
    if (auto image = dynamic_cast<ImageDataObject *>(&data))
    {
        if (QFileInfo(fileName).suffix().toLower() == "vti")
        {
            return exportVTKXMLImageData(data, fileName);
        }

        return exportImageFormat(*image, fileName);
    }

    if (auto polyData = dynamic_cast<GenericPolyDataObject *>(&data))
    {
        return exportVTKXMLPolyData(*polyData, fileName);
    }

    if (auto grid = dynamic_cast<VectorGrid3DDataObject *>(&data))
    {
        return exportVTKXMLImageData(*grid, fileName);
    }

    return false;
}

bool Exporter::isExportSupported(const DataObject & data)
{
    return formatFilters().contains(data.dataTypeName());
}

QString Exporter::formatFilter(const DataObject & data)
{
    return formatFilters().value(data.dataTypeName());
}

const QMap<QString, QString> & Exporter::formatFilters()
{
    static const QString vtps = "VTK XML PolyData Files (*.vtp)";

    static QMap<QString, QString> ff = {
        { PointCloudDataObject::dataTypeName_s(), vtps },
        { PolyDataObject::dataTypeName_s(), vtps },
        { ImageDataObject::dataTypeName_s(), "Bitmap (*.bmp);;JPEG (*.jpg  *.jpeg);;PNG (*.png);;VTK XML Image Files (*.vti)" },
        { VectorGrid3DDataObject::dataTypeName_s(), "VTK XML Image Files (*.vti)" }
    };

    return ff;
}

bool Exporter::exportImageFormat(ImageDataObject & image, const QString & fileName)
{
    QString ext = QFileInfo(fileName).suffix().toLower();

    vtkSmartPointer<vtkImageWriter> writer;

    if (ext == "png")
    {
        writer = vtkSmartPointer<vtkPNGWriter>::New();
    }
    else if (ext == "jpg" || ext == "jpeg")
    {
        writer = vtkSmartPointer<vtkJPEGWriter>::New();
    }
    else if (ext == "bmp")
    {
        writer = vtkSmartPointer<vtkBMPWriter>::New();
    }

    if (!writer)
    {
        return false;
    }

    const auto scalars = image.dataSet()->GetPointData()->GetScalars();
    if (!scalars)
    {
        return false;
    }

    const auto components = scalars->GetNumberOfComponents();
    if (components != 1 && components != 3 && components != 4)
    {
        return false;
    }

    if (scalars->GetDataType() == VTK_UNSIGNED_CHAR)
    {
        writer->SetInputData(image.dataSet());
    }
    else
    {
        auto toUChar = vtkSmartPointer<ImageMapToColors>::New();
        toUChar->SetInputData(image.dataSet());

        auto lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->SetNumberOfTableValues(0xFF);
        lut->SetHueRange(0, 0);
        lut->SetSaturationRange(0, 0);
        lut->SetValueRange(0, 1);

        ValueRange<> totalRange;
            
        for (int c = 0; c < components; ++c)
        {
            ValueRange<> range;
            scalars->GetRange(range.data(), c);
            totalRange.add(range);
        }

        toUChar->SetOutputFormat(
            components == 3 ? VTK_RGB :
            (components == 4 ? VTK_RGBA :
                VTK_LUMINANCE));

        toUChar->SetLookupTable(lut);

        writer->SetInputConnection(toUChar->GetOutputPort());
    }

    writer->SetFileName(fileName.toUtf8().data());
    writer->Write();

    return true;
}

bool Exporter::exportVTKXMLPolyData(DataObject & polyData, const QString & fileName)
{
    auto writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
    writer->SetInputData(polyData.dataSet());

    return writeVTKXML(writer, fileName);
}

bool Exporter::exportVTKXMLImageData(DataObject & image, const QString & fileName)
{
    auto writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetInputData(image.dataSet());

    return writeVTKXML(writer, fileName);
}
