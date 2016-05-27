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
    explicit GridDataImporterWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
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
};

template<> inline void GridDataImporterWidget::setImportDataType<float>()
{
    setImportDataType(VTK_FLOAT);
}

template<> inline void GridDataImporterWidget::setImportDataType<double>()
{
    setImportDataType(VTK_DOUBLE);
}
