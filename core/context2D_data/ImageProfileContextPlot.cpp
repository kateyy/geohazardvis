#include "ImageProfileContextPlot.h"

#include <cassert>

#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include <vtkPlotLine.h>
#include <vtkPointData.h>
#include <vtkTable.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/context2D_data/vtkPlotCollection.h>


using namespace reflectionzeug;


ImageProfileContextPlot::ImageProfileContextPlot(ImageProfileData * dataObject)
    : Context2DData(dataObject)
    , m_plotLine(vtkSmartPointer<vtkPlotLine>::New())
{
    connect(dataObject, &DataObject::dataChanged, this, &ImageProfileContextPlot::updatePlot);
}

PropertyGroup * ImageProfileContextPlot::createConfigGroup()
{
    PropertyGroup * root = new PropertyGroup();

    return root;
}

vtkSmartPointer<vtkPlotCollection> ImageProfileContextPlot::fetchPlots()
{
    VTK_CREATE(vtkPlotCollection, items);

    updatePlot();

    items->AddItem(m_plotLine);

    return items;
}


void ImageProfileContextPlot::visibilityChangedEvent(bool /*visible*/)
{
}

void ImageProfileContextPlot::updatePlot()
{
    ImageProfileData * profileData = static_cast<ImageProfileData *>(dataObject());

    vtkDataSet * probe = profileData->probedLine();

    vtkDataArray * probedValues = probe->GetPointData()->GetScalars();
    assert(probedValues);

    // hackish: x-values calculated by translating+rotating the probe line to the x-axis
    vtkDataSet * profilePoints = profileData->processedDataSet();

    assert(probedValues->GetNumberOfTuples() == profilePoints->GetNumberOfPoints());

    VTK_CREATE(vtkTable, table);
    VTK_CREATE(vtkFloatArray, xAxis);
    xAxis->SetNumberOfValues(profilePoints->GetNumberOfPoints());
    xAxis->SetName(profileData->abscissa().toLatin1().data());
    table->AddColumn(xAxis);
    table->AddColumn(probedValues);

    for (int i = 0; i < profilePoints->GetNumberOfPoints(); ++i)
    {
        double point[3];
        profilePoints->GetPoint(i, point);
        table->SetValue(i, 0, point[0]);
    }

    m_plotLine->SetInputData(table, 0, 1);
}
