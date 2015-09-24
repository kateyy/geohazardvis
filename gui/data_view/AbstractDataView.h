#pragma once

#include <QWidget>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class QDockWidget;
class QToolBar;
class vtkIdTypeArray;

class DataObject;
enum class IndexType;


class GUI_API AbstractDataView : public QWidget
{
    Q_OBJECT

public:
    AbstractDataView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~AbstractDataView() override;

    int index() const;

    void updateTitle(QString message = {});

    /** @return a QDockWidget, that contains the widget, to be embedded in QMainWindows
    Creates the QDockWidget instance if required. */
    QDockWidget * dockWidgetParent();
    bool hasDockWidgetParent() const;

    QToolBar * toolBar();
    bool toolBarIsVisible() const;
    void setToolBarVisible(bool visible);

    virtual bool isTable() const = 0;
    virtual bool isRenderer() const = 0;

    /** name the id and represented data */
    virtual QString friendlyName() const = 0;
    /** A title specifying contents of a sub-view */
    virtual QString subViewFriendlyName(unsigned int subViewIndex) const;

    /** highlight requested index */
    void setSelection(DataObject * dataObject, vtkIdType selectionIndex, IndexType indexType);
    void setSelection(DataObject * dataObject, vtkIdTypeArray & selectionIndices, IndexType indexType);
    void clearSelection();
    /** Selected element in the currently selected data object. For multi-selections, this returns the last index in the list.
      * (The most recently added index) */
    vtkIdType lastSelectedIndex() const;
    /** List of selected cells or points that are selected in the current data object.
      * @warning This cannot be const, due to the non-const vtkAbstractArray interface. Do not modify the contents of the returned array! */
    vtkIdTypeArray * selectedIndices();
    IndexType selectedIndexType() const;
    DataObject * selectedDataObject();
    const DataObject * selectedDataObject() const;

signals:
    /** signaled when the widget receive the keyboard focus (focusInEvent) */
    void focused(AbstractDataView * tableView);
    void closed();

    /** user selected some content */
    void objectPicked(DataObject * dataObject, vtkIdType selectionIndex, IndexType indexType);

public:
    /** highlight this view as currently selected of its type */
    void setCurrent(bool isCurrent);

protected:
    virtual QWidget * contentWidget() = 0;

    void showEvent(QShowEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;
    void closeEvent(QCloseEvent * event) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

    /** Handle selection changes in concrete sub-classes 
      * @warning selection array cannot be const, due to the non-const vtkAbstractArray interface. Do not modify its contents! */
    virtual void selectionChangedEvent(DataObject * dataObject, vtkIdTypeArray * selection, IndexType indexType) = 0;

private:
    const int m_index;
    bool m_initialized;

    QDockWidget * m_dockWidgetParent;
    QToolBar * m_toolBar;

    DataObject * m_selectedDataObject;
    vtkSmartPointer<vtkIdTypeArray> m_selectedIndices;
    IndexType m_selectionIndexType;
};
