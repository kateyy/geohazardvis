#pragma once

#include <memory>
#include <type_traits>

#include <QDialog>

#include <gui/gui_api.h>
#include <core/io/types.h>


class DataObject;
class PolyDataObject;
class Ui_AsciiImporterWidget;


class GUI_API AsciiImporterWidget : public QDialog
{
public:
    explicit AsciiImporterWidget(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~AsciiImporterWidget() override;

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

private:
    void openPointCoords();
    void openTriangleIndices();
    void updateSummary();
    bool importToPolyData();

    void clearData();

    QString getOpenFileDialog(const QString & title);

protected:
    void closeEvent(QCloseEvent * event) override;

private:
    std::unique_ptr<Ui_AsciiImporterWidget> m_ui;
    QString m_lastOpenFolder;

    int m_importDataType;
    io::InputVector m_coordinateData;
    io::InputVector m_indexData;
    std::unique_ptr<PolyDataObject> m_polyData;
};


template<typename T>
void AsciiImporterWidget::setImportDataType()
{
    if (std::is_same<T, float>())
        return setImportDataType(VTK_FLOAT);

    if (std::is_same<T, double>())
        return setImportDataType(VTK_DOUBLE);
}
