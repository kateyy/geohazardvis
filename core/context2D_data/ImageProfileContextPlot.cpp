#include "ImageProfileContextPlot.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkTable.h>
#include <vtkPointSet.h>
#include <vtkFloatArray.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/context2D_data/vtkContextItemCollection.h>

using namespace reflectionzeug;


ImageProfileContextPlot::ImageProfileContextPlot(ImageProfileData * dataObject)
    : Context2DData(dataObject)
{
}

PropertyGroup * ImageProfileContextPlot::createConfigGroup()
{
    PropertyGroup * root = new PropertyGroup();

    return root;
}

vtkSmartPointer<vtkContextItemCollection> ImageProfileContextPlot::fetchContextItems()
{
    VTK_CREATE(vtkContextItemCollection, items);

    ImageProfileData * profileData = static_cast<ImageProfileData *>(dataObject());
    vtkDataSet * profile = dataObject()->processedDataSet();

    if (!m_chart)
    {
        VTK_CREATE(vtkTable, table);
        VTK_CREATE(vtkFloatArray, xAxis);
        VTK_CREATE(vtkFloatArray, yAxis);
        xAxis->SetNumberOfValues(profile->GetNumberOfPoints());
        xAxis->SetName(profileData->abscissa().toLatin1().data());
        yAxis->SetNumberOfValues(profile->GetNumberOfPoints());
        yAxis->SetName(profileData->scalarsName().toLatin1().data());
        table->AddColumn(xAxis);
        table->AddColumn(yAxis);

        for (int i = 0; i < profile->GetNumberOfPoints(); ++i)
        {
            double point[3];
            profile->GetPoint(i, point);
            table->SetValue(i, 0, point[0]);
            table->SetValue(i, 1, point[1]);
        }

        m_chart = vtkSmartPointer<vtkChartXY>::New();
        vtkPlot * line = m_chart->AddPlot(vtkChart::LINE);
        
        line->SetInputData(table, 0, 1);
    }

    items->AddItem(m_chart);

    return items;
}


void ImageProfileContextPlot::visibilityChangedEvent(bool /*visible*/)
{
}
