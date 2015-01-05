#include "ImageProfileContextPlot.h"

#include <cassert>

#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include <vtkPen.h>
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

    m_title = dataObject->probedLine()->GetPointData()->GetScalars()->GetName();
}

PropertyGroup * ImageProfileContextPlot::createConfigGroup()
{
    PropertyGroup * root = new PropertyGroup();

    root->addProperty<std::string>("Title",
        [this] () { return title().toStdString(); },
        [this] (const std::string & s) {
        setTitle(QString::fromStdString(s));
        geometryChanged();
    });

    root->addProperty<Color>("Color",
        [this] () {
        unsigned char rgb[3];
        m_plotLine->GetColor(rgb);
        return Color(rgb[0], rgb[1], rgb[2], m_plotLine->GetPen()->GetOpacity());
    },
        [this] (const Color & color) {
        m_plotLine->SetColor(color.red(), color.green(), color.blue(), color.alpha());
        emit geometryChanged();
    });

    auto prop_transparency = root->addProperty<unsigned char>("Transparency",
        [this] () {
        return static_cast<unsigned char>((1.0f - m_plotLine->GetPen()->GetOpacity() / 255.f) * 100);
    },
        [this] (unsigned char transparency) {
        m_plotLine->GetPen()->SetOpacity(static_cast<unsigned char>(
            (100 - transparency) / 100.f * 255.f));
        emit geometryChanged();
    });
    prop_transparency->setOption("minimum", 0);
    prop_transparency->setOption("maximum", 100);
    prop_transparency->setOption("step", 1);
    prop_transparency->setOption("suffix", " %");

    auto prop_width = root->addProperty<float>("Width",
        [this] () { return m_plotLine->GetWidth(); },
        [this] (float width) {
        m_plotLine->SetWidth(width);
        emit geometryChanged();
    });
    prop_width->setOption("minimum", 0.000001f);

    return root;
}

const QString & ImageProfileContextPlot::title() const
{
    return m_title;
}

void ImageProfileContextPlot::setTitle(const QString & title)
{
    if (title == m_title)
        return;

    m_title = title;

    updatePlot();
}

vtkSmartPointer<vtkPlotCollection> ImageProfileContextPlot::fetchPlots()
{
    VTK_CREATE(vtkPlotCollection, items);

    updatePlot();

    items->AddItem(m_plotLine);

    return items;
}

void ImageProfileContextPlot::updatePlot()
{
    ImageProfileData * profileData = static_cast<ImageProfileData *>(dataObject());

    vtkDataSet * probe = profileData->probedLine();

    vtkDataArray * probedValues = probe->GetPointData()->GetScalars();
    assert(probedValues);
    probedValues->SetName(m_title.toUtf8().data());

    // hackish: x-values calculated by translating+rotating the probe line to the x-axis
    vtkDataSet * profilePoints = profileData->processedDataSet();

    assert(probedValues->GetNumberOfTuples() == profilePoints->GetNumberOfPoints());

    VTK_CREATE(vtkTable, table);
    VTK_CREATE(vtkFloatArray, xAxis);
    xAxis->SetNumberOfValues(profilePoints->GetNumberOfPoints());
    xAxis->SetName(profileData->abscissa().toUtf8().data());
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