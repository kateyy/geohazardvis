#pragma once

#include <QWidget>

#include <vtkType.h>

#include <gui/gui_api.h>


class QDockWidget;
class QToolBar;
class DataObject;


class GUI_API AbstractDataView : public QWidget
{
    Q_OBJECT

public:
    AbstractDataView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

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

    vtkIdType highlightedItemId() const;
    DataObject * highlightedObject();
    const DataObject * highlightedObject() const;

signals:
    /** signaled when the widget receive the keyboard focus (focusInEvent) */
    void focused(AbstractDataView * tableView);
    void closed();

    /** user selected some content */
    void objectPicked(DataObject * dataObject, vtkIdType selectionId);

public:
    /** highlight this view as currently selected of its type */
    void setCurrent(bool isCurrent);

    /** highlight requested id */
    void setHighlightedId(DataObject * dataObject, vtkIdType itemId);

protected:
    virtual QWidget * contentWidget() = 0;

    void showEvent(QShowEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;
    void closeEvent(QCloseEvent * event) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

    virtual void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) = 0;

private:
    const int m_index;
    bool m_initialized;

    QDockWidget * m_dockWidgetParent;
    QToolBar * m_toolBar;

    DataObject * m_highlightedObject;
    vtkIdType m_hightlightedItemId;
};
