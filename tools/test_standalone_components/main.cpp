#include <cassert>

#include <QApplication>
#include <QDebug>
#include <QScopedPointer>
#include <QPushButton>
#include <QMainWindow>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <core/io/Loader.h>
#include <core/io/Exporter.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/data_view/RenderView.h>
#include <gui/widgets/RenderConfigWidget.h>
#include <gui/widgets/RendererConfigWidget.h>
#include <gui/widgets/ColorMappingChooser.h>
#include <gui/widgets/GlyphMappingChooser.h>


int main(int argc, char **argv)
{
    QString fileName{ "C:/develop/$sync/GFZ/data/VTK XML data/Volcano 2 topo.vtp" };

    QScopedPointer<DataObject> data{ Loader::readFile(fileName) };

    QApplication app(argc, argv);
    QMainWindow window;
    window.show();

    RenderView renderView(-1);
    window.addDockWidget(Qt::TopDockWidgetArea, renderView.dockWidgetParent());
    app.setActiveWindow(&renderView);

    RenderConfigWidget renderConfig;
    renderConfig.setCurrentRenderView(&renderView);
    renderConfig.show();

    RendererConfigWidget rendererConfig;
    rendererConfig.setRenderViews({ &renderView });
    rendererConfig.setCurrentRenderView(&renderView);
    rendererConfig.show();

    ColorMappingChooser colorMapping;
    colorMapping.setCurrentRenderView(&renderView);
    colorMapping.show();

    GlyphMappingChooser glyphs;
    glyphs.setCurrentRenderView(&renderView);
    glyphs.show();


    QPushButton button;
    button.connect(&button, &QPushButton::clicked, [&renderView, &data] (){
        QList<DataObject *> incomp;
        renderView.showDataObjects({ data.data() }, incomp);
    });
    button.show();

    QPushButton exitButton("quit");
    exitButton.connect(&exitButton, &QPushButton::clicked, &app, &QApplication::quit);
    exitButton.show();

    return app.exec();

    /*auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.7, 0.7, 0.7);

    QScopedPointer<RenderedData> rendered{ data->createRendered() };

    rendered->viewProps()->InitTraversal();
    while (auto && it = rendered->viewProps()->GetNextProp())
    {
        renderer->AddViewProp(it);
    }

    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);

    auto rwInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    rwInteractor->SetRenderWindow(renderWindow);

    rwInteractor->Start();


    return 0;*/
}
