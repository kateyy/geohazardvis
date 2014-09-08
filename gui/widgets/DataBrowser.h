#pragma once

#include <QWidget>
#include <QList>


class Ui_DataBrowser;
class DataBrowserTableModel;
class DataObject;
class DataMapping;
class RenderView;


class DataBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit DataBrowser(QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~DataBrowser() override;

    void setDataMapping(DataMapping * dataMapping);

    void addDataObject(DataObject * dataObject);

protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;

private slots:
    /** show and bring to front the table for selected objects */
    void showTable();
    /** change visibility of selected objects in the current render view */
    void changeRenderedVisibility(DataObject * clickedObject);

    /** unload selected objects, free all data/settings, close views if empty */
    void removeFile();

    void evaluateItemViewClick(const QModelIndex & index);

    /** set button states for object visibilities in the renderView */
    void setupGuiFor(RenderView * renderView);

private:
    QList<DataObject *> selectedDataObjects() const;

private:
    Ui_DataBrowser * m_ui;
    DataBrowserTableModel * m_tableModel;
    DataMapping * m_dataMapping;
};
