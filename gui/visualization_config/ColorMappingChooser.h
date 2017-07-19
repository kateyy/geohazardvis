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

#include <map>
#include <memory>
#include <vector>

#include <QDockWidget>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <gui/gui_api.h>


class vtkLookupTable;
class vtkObject;

class AbstractRenderView;
class AbstractVisualizedData;
class ColorMapping;
class DataObject;
class OrientedScalarBarActor;
class Ui_ColorMappingChooser;


class GUI_API ColorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    explicit ColorMappingChooser(QWidget * parent = nullptr, Qt::WindowFlags flags = {});
    ~ColorMappingChooser() override;

    QString selectedGradientName() const;

    void setCurrentRenderView(AbstractRenderView * renderView);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);
    void setSelectedVisualization(AbstractVisualizedData * visualization);

signals:
    void renderSetupChanged();

private:
    void guiScalarsSelectionChanged();
    void guiGradientSelectionChanged();
    void guiComponentChanged(int guiComponent);
    void guiMinValueChanged(double value);
    void guiMaxValueChanged(double value);
    void guiResetMinToData();
    void guiResetMaxToData();
    void guiSelectNanColor();
    void guiLegendPositionChanged();
    void guiLegendTitleChanged();

    void rebuildGui();
    void updateScalarsSelection();
    void updateScalarsEnabled();
    void setupGuiConnections();
    void discardGuiConnections();
    void setupValueRangeConnections();
    void discardValueRangeConnections();

    /** Update the GUI-selected scalars when the mapping is modified directly via its interface */
    void mappingScalarsChanged();

    void colorLegendPositionChanged();
    void updateLegendTitleFont();
    void updateLegendLabelFont();
    void updateLegendConfig();
    void updateNanColorButtonStyle(const QColor & color);
    void updateNanColorButtonStyle(const unsigned char color[4]);

    void loadGradientImages();

    /** remove data from the UI if we currently hold it */
    void checkRemovedData(const QList<AbstractVisualizedData *> & content);

    void updateTitle();
    void updateGuiValueRanges();

    OrientedScalarBarActor & legend();

private:
    std::unique_ptr<Ui_ColorMappingChooser> m_ui;

    AbstractRenderView * m_renderView;
    std::vector<QMetaObject::Connection> m_viewConnections;
    ColorMapping * m_mapping;

    std::vector<QMetaObject::Connection> m_mappingConnections;
    bool m_inAdjustLegendPosition;
    /** Mapping from subject (color legend coordinate, text property, etc) to observer id */
    std::map<vtkWeakPointer<vtkObject>, unsigned long> m_colorLegendObserverIds;
    // connections for various parameters and signals related to the color mapping
    std::vector<QMetaObject::Connection> m_guiConnections;
    QMetaObject::Connection m_dataMinMaxChangedConnection;

private:
    Q_DISABLE_COPY(ColorMappingChooser)
};
