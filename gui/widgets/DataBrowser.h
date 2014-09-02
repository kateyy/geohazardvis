#pragma once

#include <QWidget>
#include <QList>


class Ui_DataBrowser;
class DataBrowserTableModel;
class DataObject;
class DataMapping;
class RenderWidget;


class DataBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit DataBrowser(QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~DataBrowser() override;

    void setDataMapping(DataMapping * dataMapping);

    void addDataObject(DataObject * dataObject);

private slots:
    /** show and bring to front the table for selected objects */
    void showTable();
    /** open new render view for currently selected objects */
    void openRenderView();
    /** set the visibility of selected objects in the current render view */
    void setRenderedVisibility(bool visible);

    /** unload selected objects, free all data/settings, close views if empty */
    void removeFile();

    /** set button states for object visibilities in the renderView */
    void setupGuiFor(RenderWidget * renderView);

private:
    QList<DataObject *> selectedDataObjects() const;

private:
    Ui_DataBrowser * m_ui;
    DataBrowserTableModel * m_tableModel;
    DataMapping * m_dataMapping;

    QList<DataObject *> m_dataObjects;
};
