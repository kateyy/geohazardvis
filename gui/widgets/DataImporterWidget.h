#pragma once

#include <memory>

#include <QDialog>

#include <core/io/types.h>
#include <gui/gui_api.h>


class DataObject;
class PolyDataObject;
class Ui_DataImporterWidget;


class GUI_API DataImporterWidget : public QDialog
{
public:
    /** @param showNonCsvTypesTabs Set whether to show an additional tab for importing additional data types (single polygonal files).
        This option allows to use the widget as generalized importer widget for arbitrary file types. */
    explicit DataImporterWidget(QWidget * parent = nullptr, bool showNonCsvTypesTabs = false, Qt::WindowFlags flags = {});
    ~DataImporterWidget() override;

    /** Release ownership and return the loaded poly data object.
    * @return A valid object, only if the dialog was executed and its result is QDialog::Accepted */
    std::unique_ptr<PolyDataObject> releaseLoadedPolyData();
    /** Convenience method to access the loaded poly data as its base class pointer.
    * @see releaseLoadedPolyData */
    std::unique_ptr<DataObject> releaseLoadedData();

    const QString & openFileDirectory() const;
    void setOpenFileDirectory(const QString & directory);

    /** Specify the data type for loaded coordinate values. This is VTK_FLOAT by default */
    void setImportDataType(int vtk_dataType);
    template<typename T> void setImportDataType();
    int importDataType() const;

protected:
    void closeEvent(QCloseEvent * event) override;

private:
    void openPointCoords();
    void openTriangleIndices();
    void openPolyDataFile();
    void updateSummary();
    bool importToPolyData();

    void clearData();

    QString getOpenFileDialog(const QString & title, bool requestPolyData);

private:
    std::unique_ptr<Ui_DataImporterWidget> m_ui;

    QString m_lastOpenFolder;

    int m_importDataType;
    io::InputVector m_coordinateData;
    io::InputVector m_indexData;
    std::unique_ptr<PolyDataObject> m_polyData;

private:
    Q_DISABLE_COPY(DataImporterWidget)
};

template<> inline void DataImporterWidget::setImportDataType<float>()
{
    setImportDataType(VTK_FLOAT);
}

template<> inline void DataImporterWidget::setImportDataType<double>()
{
    setImportDataType(VTK_DOUBLE);
}
