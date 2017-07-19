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

#include "RendererConfigWidget.h"

#include <cassert>
#include <limits>

#include <QMessageBox>

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextWidget.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/macros.h>
#include <core/utility/vtkcamerahelper.h>
#include <core/reflectionzeug_extension/QStringProperty.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>


using namespace reflectionzeug;

namespace
{
enum class ProjectionType
{
    parallel,
    perspective
};
}

void RendererConfigWidget::readCameraStats(vtkObject * DEBUG_ONLY(caller), unsigned long, void *)
{
    assert(m_currentRenderView && dynamic_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation()));
    // assuming synchronized cameras for now
    assert(vtkCamera::SafeDownCast(caller));
    assert(static_cast<RendererImplementationBase3D *>(&m_currentRenderView->implementation())->camera(0) == caller);

    std::function<void(AbstractProperty &)> updateFunc = [&updateFunc] (AbstractProperty & property)
    {
        if (property.isCollection())
        {
            property.asCollection()->forEach(updateFunc);
        }
        if (property.isValue())
        {
            property.asValue()->valueChanged();
        }
    };

    PropertyGroup * cameraGroup = nullptr;
    if (m_propertyRoot->propertyExists("Camera"))
    {
        cameraGroup = m_propertyRoot->group("Camera");
    }

    if (cameraGroup)
    {
        cameraGroup->forEach(updateFunc);
    }
}

std::unique_ptr<PropertyGroup> RendererConfigWidget::createPropertyGroupRenderer(AbstractRenderView * renderView, RendererImplementationBase3D * impl)
{
    auto root = std::make_unique<PropertyGroup>();

    auto prop_CoordSystem = root->addProperty<CoordinateSystemType::Value>("CoordinateSystem",
        [renderView]()
    {
        return renderView->currentCoordinateSystem().type;
    },
        [renderView, this] (CoordinateSystemType::Value type)
    {
        auto spec = renderView->currentCoordinateSystem();
        if (spec.type == type)
        {
            // work-around strange libzeug behavior
            return;
        }

        if (!spec.isValid())
        {
            // Switching from unspecified to some valid coordinate system type.
            // Try to set more information.
            auto && dataObjects = renderView->dataObjects();
            const auto it = std::find_if(dataObjects.cbegin(), dataObjects.cend(),
                [] (DataObject * dataObject)
            {
                return dynamic_cast<CoordinateTransformableDataObject *>(dataObject) != nullptr;
            });
            if (it != dataObjects.cend())
            {
                spec = static_cast<CoordinateTransformableDataObject *>(*it)->coordinateSystem();
            }
        }

        if (type == CoordinateSystemType::unspecified)
        {
            spec = {};
        }
        spec.type = type;

        if ((spec.type == CoordinateSystemType::metricLocal
            || spec.type == CoordinateSystemType::metricGlobal)
            && spec.unitOfMeasurement.isEmpty())
        {
            spec.unitOfMeasurement = "km";
        }
        else if (spec.type == CoordinateSystemType::geographic)
        {
            spec.unitOfMeasurement.clear();
        }

        if (!renderView->setCurrentCoordinateSystem(spec))
        {
            QMessageBox::warning(nullptr, "Coordinate System Selection",
                "The currently shown data sets cannot be shown in the selected coordinate system (" + CoordinateSystemType(type).toString() + ").");
            return;
        }
        renderView->render();
    });
    prop_CoordSystem->setOptions({
        { "title", "Coordinate System" }
    });
    std::map<CoordinateSystemType::Value, std::string> coordSystemTypeStrings;
    for (auto & pair : CoordinateSystemType::typeToStringMap())
    {
        coordSystemTypeStrings.emplace(pair.first, pair.second.toStdString());
    }
    prop_CoordSystem->setStrings(coordSystemTypeStrings);

    if (renderView->numberOfSubViews() == 1)
    {
        root->addProperty<QString>("Title",
            [impl] () { return QString::fromUtf8(impl->titleWidget(0)->GetTextActor()->GetInput()); },
            [impl, renderView] (const QString & title)
        {
            impl->titleWidget(0)->GetTextActor()->SetInput(title.toUtf8().data());
            renderView->render();
        });
    }
    else
    {
        auto titlesGroup = root->addGroup("Titles");

        for (unsigned i = 0; i < renderView->numberOfSubViews(); ++i)
        {
            titlesGroup->addProperty<QString>("Title" + std::to_string(i), [impl, i] ()
            {
                return QString::fromUtf8(impl->titleWidget(i)->GetTextActor()->GetInput());
            },
                [impl, renderView, i] (const QString & title)
            {
                impl->titleWidget(i)->GetTextActor()->SetInput(title.toUtf8().data());
                renderView->render();
            })
                ->setOption("title", "Title " + std::to_string(i + 1));
        }
    }

    auto firstRenderer = impl->renderer(0);

    root->addProperty<Color>("backgroundColor",
        [firstRenderer] () {
        double * color = firstRenderer->GetBackground();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [renderView, impl] (const Color & color) {
        for (unsigned i = 0; i < renderView->numberOfSubViews(); ++i)
        {
            impl->renderer(i)->SetBackground(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        }
        renderView->render();
    })
        ->setOption("title", "Background Color");

    const bool is3DView = renderView->implementation().currentInteractionStrategy() == "3D terrain";

    auto cameraGroup = root->addGroup("Camera");
    {
        // assuming synchronized cameras for now
        vtkCamera & camera = *impl->camera(0);

        if (is3DView)
        {
            std::string degreesSuffix = (QString(' ') + QChar(0xB0)).toStdString();

            cameraGroup->addProperty<double>("Azimuth",
                [&camera]() {
                return TerrainCamera::getAzimuth(camera);
            },
                [&camera, impl] (const double & azimuth) {
                TerrainCamera::setAzimuth(camera, azimuth);
                impl->renderer(0)->ResetCameraClippingRange();
                impl->render();
            })
                ->setOptions({
                    { "minimum", std::numeric_limits<double>::lowest() },
                    { "suffix", degreesSuffix }
            });

            cameraGroup->addProperty<double>("Elevation",
                [&camera]() {
                return TerrainCamera::getVerticalElevation(camera);
            },
                [&camera, impl] (const double & elevation) {
                TerrainCamera::setVerticalElevation(camera, elevation);
                impl->renderer(0)->ResetCameraClippingRange();
                impl->render();
            })
                ->setOptions({
                    { "minimum", -87 },
                    { "maximum", 87 },
                    { "suffix", degreesSuffix }
            });
        }

        auto prop_focalPoint = cameraGroup->addProperty<std::array<double, 3>>("focalPoint",
            [&camera](size_t component) {
            return camera.GetFocalPoint()[component];
        },
            [&camera, impl](size_t component, const double & value) {
            double foc[3];
            camera.GetFocalPoint(foc);
            foc[component] = value;
            camera.SetFocalPoint(foc);
            impl->renderer(0)->ResetCameraClippingRange();
            impl->render();
        });
        prop_focalPoint->setOption("title", "Focal Point");
        char title[2] {'X', 0};
        for (quint8 i = 0; i < 3; ++i)
        {
            prop_focalPoint->at(i)->setOptions({
                { "title", std::string(title) },
                { "minimum", std::numeric_limits<float>::lowest() } });
            ++title[0];
        }

        if (is3DView)
        {
            cameraGroup->addProperty<double>("Distance",
                [&camera] () { return camera.GetDistance(); },
                [&camera, impl] (double d) {

                /** work around bug (?) in vtkCamera (not considering the distance for clipping plane calculation)
                  * @see RendererImplementationBase3D::resetClippingRanges */
                if (camera.GetParallelProjection() != 0)
                {
                    return;
                }

                TerrainCamera::setDistanceFromFocalPoint(camera, d);

                impl->renderer(0)->ResetCameraClippingRange();
                impl->render();
            })
                ->setOption("minimum", 0.001f);

            cameraGroup->addProperty<ProjectionType>("Projection",
                [&camera] () {
                return camera.GetParallelProjection() != 0 ? ProjectionType::parallel : ProjectionType::perspective;
            },
                [&camera, impl] (ProjectionType type) {
                camera.SetParallelProjection(type == ProjectionType::parallel);
                impl->renderer(0)->ResetCameraClippingRange();
                impl->render();
            })
                ->setStrings({
                    { ProjectionType::parallel, "parallel" },
                    { ProjectionType::perspective, "perspective" }
            });
        }

        cameraGroup->addProperty<double>("parallelScale",
            [&camera] () { return camera.GetParallelScale(); },
            [&camera, impl] (double s) {

            camera.SetParallelScale(s);

            impl->render();
        })
            ->setOptions({
                { "title", "Parallel Scale" },
                { "minimum", 0.001f }
        });
    }

    createPropertyGroupRenderer2(*root, renderView, impl);

    return root;
}
