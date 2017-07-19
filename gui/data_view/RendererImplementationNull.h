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

#pragma once

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkRenderer;


class GUI_API RendererImplementationNull : public RendererImplementation
{
public:
    explicit RendererImplementationNull(AbstractRenderView & renderView);
    ~RendererImplementationNull() override;

    QString name() const override;
    ContentType contentType() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) override;

    void activate(t_QVTKWidget & qvtkWidget) override;
    void deactivate(t_QVTKWidget & qvtkWidget) override;

    void render() override;
    vtkRenderWindowInteractor * interactor() override;

    void lookAtData(const VisualizationSelection & selection, unsigned int subViewIndex) override;
    void resetCamera(bool toInitialPosition, unsigned int subViewIndex) override;

    void setAxesVisibility(bool) override;
    bool canApplyTo(const QList<DataObject *> &) override;

protected:
    std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject &) const override;
    void onAddContent(AbstractVisualizedData *, unsigned int) override;
    void onRemoveContent(AbstractVisualizedData *, unsigned int) override;

    void onSetSelection(const VisualizationSelection & selection) override;
    void onClearSelection() override;

private:
    vtkSmartPointer<vtkRenderer> m_renderer;

private:
    Q_DISABLE_COPY(RendererImplementationNull)
};
