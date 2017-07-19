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

#include <memory>
#include <vector>

#include <QDockWidget>

#include <gui/gui_api.h>


namespace reflectionzeug
{
    class PropertyGroup;
}
class Ui_RenderPropertyConfigWidget;
class AbstractRenderView;
class AbstractVisualizedData;
class DataObject;
class TemporalPipelineMediator;
struct VisualizationSelection;


class GUI_API RenderPropertyConfigWidget : public QDockWidget
{
public:
    explicit RenderPropertyConfigWidget(QWidget * parent = nullptr);
    ~RenderPropertyConfigWidget() override;

public:
    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);
    void setSelectedData(DataObject * dataObject);
    void setSelectedVisualization(AbstractRenderView * renderView, const VisualizationSelection & selection);
    void clear();

    void setStartDate(int timeStepIndex);
    void setEndDate(int timeStepIndex);

private:
    /** remove data from the UI if we currently hold it */
    void checkDeletedContent(const QList<AbstractVisualizedData *> & content);

    void updateTitle();

    template<typename T1, typename T1Func, typename T2, typename T2Func>
    void guiConnect(T1 * obj1, T1Func func1, T2 * obj2, T2Func func2);

private:
    std::unique_ptr<Ui_RenderPropertyConfigWidget> m_ui;
    std::unique_ptr<TemporalPipelineMediator> m_temporalSelector;
    std::vector<QMetaObject::Connection> m_guiConnections;

    std::unique_ptr<reflectionzeug::PropertyGroup> m_propertyRoot;

    AbstractRenderView * m_renderView;
    AbstractVisualizedData * m_content;

private:
    Q_DISABLE_COPY(RenderPropertyConfigWidget)
};

template<typename T1, typename T1Func, typename T2, typename T2Func>
void RenderPropertyConfigWidget::guiConnect(T1 * obj1, T1Func func1, T2 * obj2, T2Func func2)
{
    m_guiConnections.emplace_back(connect(obj1, func1, obj2, func2));
}
