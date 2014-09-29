#pragma once

#include <QDockWidget>

#include <vtkType.h>


class DataObject;


class AbstractDataView : public QDockWidget
{
    Q_OBJECT

public:
    AbstractDataView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

    int index() const;

    void updateTitle(QString message = {});

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

public slots:
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

    DataObject * m_highlightedObject;
    vtkIdType m_hightlightedItemId;
};
