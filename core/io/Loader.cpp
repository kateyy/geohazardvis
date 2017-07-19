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

#include "Loader.h"

#include <algorithm>
#include <cassert>

#include <QFileInfo>
#include <QDebug>

#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDEMReader.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkImageReader2.h>
#include <vtkImageReader2Factory.h>
#include <vtkNew.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPolyDataReader.h>

#include <core/data_objects/GenericPolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/io/DeformationTimeSeriesTextFileReader.h>
#include <core/io/io_helper.h>
#include <core/io/MetaTextFileReader.h>


std::unique_ptr<DataObject> Loader::readFile(const QString & filename)
{
    {
        QFile f(filename);
        if (!f.exists())
        {
            qDebug() << "Loader: trying to open non-existing file: " << filename;
            return nullptr;
        }
        if (!f.open(QIODevice::ReadOnly))
        {
            qDebug() << "Loader: cannot open file for read-only access: " << filename;
            return nullptr;
        }
    }

    QFileInfo fileInfo{ filename };
    QString baseName = fileInfo.baseName();
    QString ext = fileInfo.suffix().toLower();

    auto getName = [] (vtkDataSet * dataSet) -> QString
    {
        auto nameArray = vtkCharArray::FastDownCast(dataSet->GetFieldData()->GetAbstractArray("Name"));
        if (!nameArray)
        {
            return "";
        }

        return QString::fromUtf8(
            nameArray->GetPointer(0),
            static_cast<int>(nameArray->GetSize()));
    };

    if (ext == "vti" || ext == "dem")
    {
        vtkSmartPointer<vtkImageData> image;

        if (ext == "vti")
        {
            vtkNew<vtkXMLImageDataReader> reader;
            reader->SetFileName(filename.toUtf8().data());

            if (reader->GetExecutive()->Update() == 1)
            {
                image = reader->GetOutput();
            }

            if (!image)
            {
                qDebug() << "Invalid VTK image file: " << filename;
                return nullptr;
            }
        }
        else if (ext == "dem")
        {
            vtkNew<vtkDEMReader> reader;
            reader->SetFileName(filename.toUtf8().data());

            if (reader->GetExecutive()->Update() == 1)
            {
                image = reader->GetOutput();
            }

            if (!image)
            {
                qDebug() << "Invalid DEM file: " << filename;
                return nullptr;
            }
        }

        assert(image);

        QString dataSetName = getName(image);
        if (dataSetName.isEmpty())
        {
            vtkDataArray * data = nullptr;
            if (!(data = image->GetPointData()->GetScalars()))
            {
                if (!(data = image->GetPointData()->GetVectors()))
                {
                    if (!(data = image->GetCellData()->GetScalars()))
                    {
                        data = image->GetCellData()->GetVectors();
                    }
                }
            }
            if (data && data->GetName())
            {
                dataSetName = QString::fromUtf8(data->GetName());
            }
        }
        if (dataSetName.isEmpty())
        {
            dataSetName = baseName;
        }

        switch (image->GetDataDimension())
        {
        case 2:
            return std::make_unique<ImageDataObject>(dataSetName, *image);
        case 3:
            return std::make_unique<VectorGrid3DDataObject>(dataSetName, *image);

        default:
            qDebug() << "VTK image data format not supported.";
            return nullptr;
        }
    }

    if (ext == "vtp")
    {
        auto reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        reader->SetFileName(filename.toUtf8().data());

        vtkSmartPointer<vtkPolyData> polyData;
        if (reader->GetExecutive()->Update() == 1)
        {
            polyData = reader->GetOutput();
        }

        if (!polyData)
        {
            qDebug() << "Invalid VTK PolyData file: " << filename;
            return nullptr;
        }

        QString dataSetName = getName(polyData);
        if (dataSetName.isEmpty())
        {
            vtkDataArray * data = nullptr;
            if (!(data = polyData->GetCellData()->GetScalars()))
            {
                if (!(data = polyData->GetPointData()->GetScalars()))
                {
                    if (!(data = polyData->GetCellData()->GetVectors()))
                    {
                        data = polyData->GetPointData()->GetVectors();
                    }
                }
            }
            if (data && data->GetName())
            {
                dataSetName = QString::fromUtf8(data->GetName());
            }
        }
        if (dataSetName.isEmpty())
        {
            dataSetName = baseName;
        }

        auto instance = GenericPolyDataObject::createInstance(dataSetName, *polyData);
        if (!instance)
        {
            qDebug() << "Invalid VTK PolyData file: " << filename;
            return nullptr;
        }
        return std::move(instance);
    }

    auto && vtkImageExts = io::fileFormatExtensions(io::Category::VTKImageFormats);
    if (vtkImageExts.find(ext) != vtkImageExts.end())
    {
        vtkSmartPointer<vtkImageReader2> reader;
        reader.TakeReference(
            vtkImageReader2Factory::CreateImageReader2(filename.toUtf8().data()));

        if (!reader)
        {
            qDebug() << "Unsupported image format: " << filename;
            return nullptr;
        }

        vtkImageData * image = nullptr;
        reader->SetFileName(filename.toUtf8().data());
        if (reader->GetExecutive()->Update() == 1)
        {
            image = reader->GetOutput();
        }

        if (!image)
        {
            qDebug() << "Invalid image file: " << filename;
            return nullptr;
        }

        auto scalars = image->GetPointData()->GetScalars();
        // readers set the scalar name to something like "JPEGdata"..
        if (scalars)
        {
            scalars->SetName(baseName.toUtf8().data());
        }

        return std::make_unique<ImageDataObject>(baseName, *image);
    }

    {
        DeformationTimeSeriesTextFileReader deformationReader(filename);
        const auto state = deformationReader.readInformation();
        if (state == DeformationTimeSeriesTextFileReader::validInformation)
        {
            if (deformationReader.readData() != DeformationTimeSeriesTextFileReader::validData)
            {
                qDebug() << "Invalid deformation file: " << filename;
                return nullptr;
            }
            return deformationReader.generateDataObject();
        }
        qDebug() << "Text file not recognized as valid deformation time series file.";
    }

    // handle all other files as our text file format
    return MetaTextFileReader::read(filename);
}
