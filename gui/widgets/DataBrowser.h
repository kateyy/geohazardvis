#pragma once

#include <QWidget>
#include <QScopedPointer>


template<typename T> class QList;

class Ui_DataBrowser;
class AbstractRenderView;
class DataBrowserTableModel;
class DataObject;
class DataMapping;


class DataBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit DataBrowser(QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~DataBrowser() override;

    void setDataMapping(DataMapping * dataMapping);

    void setSelectedData(DataObject * data);
    QList<DataObject *> selectedDataObjects() const;
    QList<DataObject *> selectedDataSets() const;
    QList<DataObject *> selectedAttributeVectors() const;

signals:
    void selectedDataChanged(DataObject * data);

protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    void updateModelForFocusedView();
    void updateModel(AbstractRenderView * renderView);

    /** show and bring to front the table for selected objects */
    void showTable();
    /** change visibility of selected objects in the current render view */
    void changeRenderedVisibility(DataObject * clickedObject);

    void menuAssignDataToIndexes(const QPoint & position, DataObject * clickedData);

    /** unload selected objects, free all data/settings, close views if empty */
    void removeFile();

    void evaluateItemViewClick(const QModelIndex & index, const QPoint & position);

private:
    QScopedPointer<Ui_DataBrowser> m_ui;
    DataBrowserTableModel * m_tableModel;
    DataMapping * m_dataMapping;
};
