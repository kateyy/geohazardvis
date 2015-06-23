#include <cassert>

#include <QDebug>
#include <QScopedPointer>

#include <vtkImageData.h>

#include <core/utility/vtkhelper.h>
#include <core/io/Loader.h>
#include <core/io/Exporter.h>
#include <core/data_objects/DataObject.h>


int main()
{
    QString fileName{ "E:/Users/Karsten/Documents/Studium/GFZ/data/VTK XML data/Lazufre dem_90msrtm__raster.vti" };
    QString exportFN{ "E:/Users/Karsten/Documents/Studium/GFZ/data/VTK XML data/Lazufre dem_90msrtm__raster_fixed.vti" };

    auto data = Loader::readFile(fileName);

    vtkSmartPointer<vtkImageData> image = vtkImageData::SafeDownCast(data->dataSet());

    double threshold = 6000;

    int extent[6];
    image->GetExtent(extent);

    for (int x = extent[0]; x < extent[1]; ++x)
        for (int y = extent[2]; y < extent[3]; ++y)
        {
            double v = image->GetScalarComponentAsDouble(x, y, 0, 0);

            if (v <= threshold)
                continue;


            unsigned num = 0;
            double v_fixed = 0;
            // to left
            for (int i = x - 1; i >= extent[0]; --i)
            {
                double _v = image->GetScalarComponentAsDouble(i, y, 0, 0);
                if (_v <= threshold)
                {
                    v_fixed += _v;
                    ++num;
                    break;
                }
            }
            // to right
            for (int i = x + 1; i <= extent[1]; ++i)
            {
                double _v = image->GetScalarComponentAsDouble(i, y, 0, 0);
                if (_v <= threshold)
                {
                    v_fixed += _v;
                    ++num;
                    break;
                }
            }
            // to top
            for (int i = y - 1; i >= extent[2]; --i)
            {
                double _v = image->GetScalarComponentAsDouble(x, i, 0, 0);
                if (_v <= threshold)
                {
                    v_fixed += _v;
                    ++num;
                    break;
                }
            }
            // to bottom
            for (int i = y + 1; i <= extent[3]; ++i)
            {
                double _v = image->GetScalarComponentAsDouble(x, i, 0, 0);
                if (_v <= threshold)
                {
                    v_fixed += _v;
                    ++num;
                    break;
                }
            }

            if (num == 0)
            {
                qDebug() << "Couldn't find a sourrounding value to fix " << x << " " << y;
                continue;
            }

            v_fixed /= num;

            image->SetScalarComponentFromDouble(x, y, 0, 0, v_fixed);

            qDebug() << "Fixed " << x << " " << y << " to " << v_fixed;
        }

    qDebug() << Exporter::exportData(data.get(), exportFN);

    return 0;
}
