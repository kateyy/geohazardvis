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
#include <QList>

#include <gui/gui_api.h>


class QItemSelection;

namespace reflectionzeug
{
    class PropertyGroup;
}

class AbstractRenderView;
class AbstractVisualizedData;
class GlyphMapping;
class GlyphMappingChooserListModel;
class DataObject;
class Ui_GlyphMappingChooser;


class GUI_API GlyphMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    GlyphMappingChooser(QWidget * parent = nullptr, Qt::WindowFlags flags = {});
    ~GlyphMappingChooser() override;

    void setCurrentRenderView(AbstractRenderView * renderView);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);

signals:
    void renderSetupChanged();

private:
    void updateGuiForSelection(const QItemSelection & selection);
    void updateVectorsList();

    /** remove data from the UI if we currently hold it */
    void checkRemovedData(const QList<AbstractVisualizedData *> & content);

    void updateTitle();

private:
    std::unique_ptr<Ui_GlyphMappingChooser> m_ui;

    AbstractRenderView * m_renderView;
    std::vector<QMetaObject::Connection> m_viewConnections;
    GlyphMapping * m_mapping;
    QMetaObject::Connection m_vectorListConnection;
    std::vector<QMetaObject::Connection> m_vectorsRenderConnections;
    GlyphMappingChooserListModel * m_listModel;
    std::vector<std::unique_ptr<reflectionzeug::PropertyGroup>> m_propertyGroups;

private:
    Q_DISABLE_COPY(GlyphMappingChooser)
};
