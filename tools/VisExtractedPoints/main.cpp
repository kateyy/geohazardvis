#include <cassert>

#include <QDebug>
#include <QApplication>

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkExtractSelection.h>
#include <vtkExtractSelectedPolyDataIds.h>
#include <vtkIdTypeArray.h>
#include <vtkProperty.h>
#include <vtkSelectionNode.h>
#include <vtkSelection.h>
#include <vtkVector.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/io/Loader.h>
#include <core/filters/LineOnCellsSelector2D.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/utility/vtkvectorhelper.h>

#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/widgets/ColorMappingChooser.h>
#include <gui/widgets/RenderConfigWidget.h>
#include <gui/widgets/RendererConfigWidget.h>



int main(int argc, char ** argv)
{
    QString volcR10("C:/develop/$sync/GFZ/data/VTK XML data/Volcano 2 topo.vtp");


    auto object = Loader::readFile(
        volcR10
        );

    double bounds[6];
    auto & dataSet = *object->dataSet();
    dataSet.GetBounds(bounds);

    vtkVector2d A{ bounds[0], 0 };
    vtkVector2d B{ bounds[1], 0 };

    auto inputPolyData = dynamic_cast<PolyDataObject *>(object.get());

    auto selector = vtkSmartPointer<LineOnCellsSelector2D>::New();
    selector->SetInputData(&dataSet);
    selector->SetCellCentersConnection(inputPolyData->cellCentersOutputPort());
    selector->SetStartPoint(A);
    selector->SetEndPoint(B);

    //auto extractSelection = vtkSmartPointer<vtkExtractSelectedPolyDataIds>::New();
    //extractSelection->SetInputData(&dataSet);
    //extractSelection->SetInputConnection(1, selector->GetOutputPort());

    //extractSelection->Update();

    //auto vPoly = extractSelection->GetOutput();

    //auto extractSelection = vtkSmartPointer<vtkExtractSelection>::New();
    //extractSelection->SetInputData(&dataSet);
    //extractSelection->SetSelectionConnection(selector->GetOutputPort());
    //extractSelection->Update();
    //auto outputData = vtkUnstructuredGrid::SafeDownCast(extractSelection->GetOutput());

    //auto vPoly = vtkSmartPointer<vtkPolyData>::New();
    //vPoly->SetPoints(outputData->GetPoints());
    //vPoly->SetPolys(outputData->GetCells());
    //vPoly->GetCellData()->PassData(outputData->GetCellData());
    //vPoly->GetPointData()->PassData(outputData->GetPointData());

    selector->Update();
    auto vPoly = selector->GetExtractedPoints();

    auto poly = std::make_unique<PolyDataObject>("extraction", *vPoly);

    QApplication app(argc, argv);

    ColorMappingChooser cmc;
    cmc.show();
    RenderConfigWidget rcw;
    rcw.show();
    RendererConfigWidget rrcw;
    rrcw.show();

    DataSetHandler dsh;
    DataMapping dm(dsh);
    auto view = dm.openInRenderView({ poly.get(), object.get() });

    auto pointsVis = dynamic_cast<RenderedPolyData *>(view->visualizationFor(poly.get()));
    pointsVis->mainActor()->GetProperty()->SetPointSize(5);
    pointsVis->mainActor()->PickableOn();

    auto baseVis = dynamic_cast<RenderedPolyData *>(view->visualizationFor(object.get()));
    baseVis->mainActor()->GetProperty()->SetOpacity(0.2);
    baseVis->mainActor()->PickableOff();

    view->show();
    cmc.setCurrentRenderView(view);
    rcw.setCurrentRenderView(view);
    rcw.setSelectedData(poly.get());
    rrcw.setCurrentRenderView(view);

    return app.exec();
}
