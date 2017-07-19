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

#include <functional>
#include <memory>
#include <vector>

#include <QDialog>
#include <QPointer>

#include <vtkSmartPointer.h>

#include <core/types.h>
#include <gui/gui_api.h>


class vtkAbstractArray;
class DataObject;
class DataProfile2DDataObject;
class ImageDataObject;
class PolyDataObject;
class Ui_CSVExporterWidget;


class GUI_API CSVExporterWidget : public QDialog
{
public:
    CSVExporterWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~CSVExporterWidget() override;

    void setDataObject(DataObject * dataObject);
    DataObject * dataObject();
    void clear();

    void setDir(const QString & dir);
    const QString & dir() const;

    void exportData();

    void setShowCancelAllButton(bool showCancelAll);
    bool showCancelAllButton() const;
    bool allCanceled() const;

private:
    void updateAttributesForGeometrySelection(bool checked);
    void updateAttributes();

    /**
     * Link from named attributes listed for the user in the UI to a function that extracts the
     * data as vtkAbstractArray from the data object. Internally, attributes are identified by
     * index, not by name, preventing name collisions.
     */
    struct ExportableAttribute
    {
        QString label;
        bool selectedByDefault;
        std::function<vtkSmartPointer<vtkAbstractArray>()> dataExtractor;
    };

    enum class GeometryType
    {
        points,
        triangles,
        scalarGrid2D,
        invalid
    };
    static IndexType geometryToIndexType(GeometryType geometryType);

    QString delimiterFromUi() const;
    QString lineEndingFromUi() const;

private:
    std::unique_ptr<Ui_CSVExporterWidget> m_ui;
    QPointer<DataObject> m_dataObject;
    QPointer<DataProfile2DDataObject> m_profileDataObject;
    QPointer<ImageDataObject> m_imageDataObject;
    QPointer<PolyDataObject> m_polyDataObject;
    QString m_dir;
    bool m_allCanceled;

    GeometryType m_selectedGeometry;
    std::vector<ExportableAttribute> m_attributes;

private:
    Q_DISABLE_COPY(CSVExporterWidget)
};
