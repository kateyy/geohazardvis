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

#include <QDialog>

#include <core/io/types.h>
#include <gui/gui_api.h>


class DataObject;
class ImageDataObject;
class Ui_GridDataImporterWidget;


class GUI_API GridDataImporterWidget : public QDialog
{
public:
    explicit GridDataImporterWidget(QWidget * parent = nullptr, Qt::WindowFlags flags = {});
    ~GridDataImporterWidget() override;

    std::unique_ptr<ImageDataObject> releaseLoadedImageData();
    std::unique_ptr<DataObject> releaseLoadedData();

    const QString & openFileDirectory() const;
    void setOpenFileDirectory(const QString & directory);

    /** Specify the data type for loaded values. This is VTK_FLOAT by default */
    void setImportDataType(int vtk_dataType);
    template<typename T> void setImportDataType();
    int importDataType() const;

protected:
    void closeEvent(QCloseEvent * event) override;

private:
    void openImageDataFile();
    void updateSummary();
    bool importToImageData();

    void clearData();

    QString getOpenFileDialog(const QString & title);

private:
    std::unique_ptr<Ui_GridDataImporterWidget> m_ui;

    QString m_lastOpenFolder;

    int m_importDataType;
    io::InputVector m_imageDataVector;
    std::unique_ptr<ImageDataObject> m_imageData;

private:
    Q_DISABLE_COPY(GridDataImporterWidget)
};

template<> inline void GridDataImporterWidget::setImportDataType<float>()
{
    setImportDataType(VTK_FLOAT);
}

template<> inline void GridDataImporterWidget::setImportDataType<double>()
{
    setImportDataType(VTK_DOUBLE);
}
