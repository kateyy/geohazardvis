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

#include <core/DataSetHandler.h>
#include <core/io/Loader.h>
#include <core/io/Exporter.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/data_view/RenderView.h>
#include <gui/visualization_config/ColorMappingChooser.h>
#include <gui/visualization_config/GlyphMappingChooser.h>
#include <gui/visualization_config/RendererConfigWidget.h>
#include <gui/visualization_config/RenderPropertyConfigWidget.h>


int main(int argc, char **argv)
{
    QString fileName{ "C:/develop/$sync/GFZ/data/VTK XML data/Volcano 2 topo.vtp" };

    auto data = Loader::readFile(fileName);

    QApplication app(argc, argv);
    QMainWindow window;
    window.show();

    DataSetHandler dataSetHandler;
    DataMapping dataMapping(dataSetHandler);

    RenderView renderView(dataMapping, -1);
    window.addDockWidget(Qt::TopDockWidgetArea, renderView.dockWidgetParent());
    app.setActiveWindow(&renderView);

    RenderPropertyConfigWidget renderConfig;
    renderConfig.setCurrentRenderView(&renderView);
    renderConfig.show();

    RendererConfigWidget rendererConfig;
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
        renderView.showDataObjects({ data.get() }, incomp);
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
